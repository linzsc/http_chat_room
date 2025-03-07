#include "server.h"
#include "EpollReactor.h"
#include "http_header.h"
#include "base64.h"
#include "sha1.h"
#include "logger.h"
#include <set>
/*
前端先发送 注册、登录http请求，
但后端核实身份信息后，返回成功的响应
然后，前端发送websocket升级请求
后端生成一个唯一的session_id，并返回给前端
前端将session_id保存到cookie中
前端每次发送消息时，都带上session_id
后端收到消息后，根据session_id验证客户端身份
根据message中的type字段，选择群发和私聊

*/
#define PORT 12345

// 定义回调函数类型
std::set<int>online_members;

// 定义客户端上下文
struct ClientContext {
    std::string buffer;
    int buffer_pos;
    bool is_websocket; // 标识是否是WebSocket连接
    bool is_logged_in; // 标识是否已登录
    std::string ws_fragment_buffer;
    ClientContext() : buffer_pos(0), is_websocket(false) {}
};

// Epoll Reactor 服务器

EpollReactor server;

// 全局变量
std::unordered_map<int, ClientContext> client_contexts;

// 发送 HTTP 响应


// 处理用户注册
void user_register(int fd, std::string ctx_body) {

    SimpleJSONParser parser(ctx_body);
    std::map<std::string, std::string> user_info=parser.parse();
    std::string username=user_info["username"],password=user_info["password"] ;
    LOG_INFO("register request: "+username+", "+password);
    UserDAO userDAO;

    // 测试注册
    bool registerSuccess = userDAO.registerUser(username, password);
    if (registerSuccess) {
        
        LOG_INFO("User registered successfully.");
        std::map<std::string, std::string> extraHeaders = {{"Content-Type", "application/json"}};
        HttpParser::sendHttpResponse(fd, HttpStatus::OK, extraHeaders , "{\"success\": true, \"message\": \"Regist successful\"}");
    } else {
        
        LOG_ERROR("Failed to register user.");
        std::map<std::string, std::string> extraHeaders = {{"Content-Type", "application/json"}};
        HttpParser::sendHttpResponse(fd,HttpStatus::NOT_FOUND,  extraHeaders, "{\"status\":\"这个id已经注册过了！\"}");
    }

}

// 处理用户登录
void user_login(int fd, std::string ctx_body)
{
    
    SimpleJSONParser parser(ctx_body);
    std::map<std::string, std::string> user_info=parser.parse();

    std::string username=user_info["username"],password=user_info["password"] ;
    
    LOG_INFO("Login request: "+username+", "+password);
    UserDAO userDAO;
    int userId = userDAO.verifyLogin(username, password);
    if (userId != -1) {
        

        LOG_INFO("Login successful. User name: "+username);
        std::map<std::string, std::string> extraHeaders = {{"Content-Type", "application/json"}};
       
        HttpParser::sendHttpResponse(fd, HttpStatus::OK, extraHeaders, "{\"success\": true, \"message\": \"Login successful\", \"userId\": \"" + std::to_string(userId) + "\"}");
    } else {
        std::cerr << "Login failed." << std::endl;
        LOG_ERROR("Login failed. User name:"+username);
        std::map<std::string, std::string> extraHeaders = {{"Content-Type", "text/html"}};
        HttpParser::sendHttpResponse(fd,  HttpStatus::NOT_FOUND, extraHeaders, "Unauthorized");
    }
        
}

void senWebSocketMessage(int fd, std::string &payload) {
    std::cout << "Sending WebSocket message: " << payload << std::endl;
    unsigned char frame[1024]; // 临时缓冲区
    size_t frame_len = 0;

    // 写入帧头
    frame[frame_len++] = 0x81; // text frame, fin=1

    // 处理负载长度
    size_t payload_len = payload.size();
    if (payload_len < 126) {
        frame[frame_len++] = (unsigned char) payload_len;
    } else if (payload_len < 65536) {
        frame[frame_len++] = 126;
        frame[frame_len++] = (payload_len >> 8) & 0xFF;
        frame[frame_len++] = payload_len & 0xFF;
    } else {
        frame[frame_len++] = 127;
        // 处理 64 位长度，这里简化
        for (int i = 0; i < 8; ++i) {
            frame[frame_len++] = (payload_len >> (56 - i * 8)) & 0xFF;
        }
    }

    // 服务器发送的帧不需要加掩码，所以 mask=0
    frame[1] &= 0x7F; // 确保 mask=0

    // 添加 payload
    memcpy(frame + frame_len, payload.data(), payload.size());
    frame_len += payload.size();

    // 发送帧
    std::cout << "Sending WebSocket frame with length: " << frame_len << std::endl;
    send(fd, frame, frame_len, 0);
}
//广播消息

void parse_msg(std::string message){
    SimpleJSONParser msg(message);
  
    std::map<std::string, std::string> msg_map=msg.parse();
    //将消息保存到数据库
    MessageDAO messageDAO;
    if(messageDAO.saveMessage(msg_map["sender_id"], msg_map["sender_name"], msg_map["content"])){
        LOG_INFO("Message saved to database successfully.");
    }
    else{
        LOG_ERROR("Failed to save message to database.");
    }

}

void handleWebSocketMessage(int fd, std::string message) {
   LOG_DEBUG("Received WebSocket message: "+message+"\nfrom client: "+std::to_string(fd)); 


    parse_msg(message);
    // 广播消息
    for (auto it = online_members.begin(); it != online_members.end(); ++it) {
        int client_fd = *it;
        if(client_fd != fd){
            senWebSocketMessage(fd, message);
            LOG_DEBUG("Broadcasting message to client: "+std::to_string(client_fd));
        }
            
    }
}
void handle_options_request(int clientSocket) {
    std::string response =
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
        "Access-Control-Allow-Headers: *\r\n"
        "Content-Length: 0\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    //std::cout<<"Sending  Option response: \n"<<response<<std::endl;
    send(clientSocket, response.c_str(), response.size(), 0);
}


void handleWebSocketFrame(int fd, const std::string& frame) {
    // 解析 WebSocket 帧
    if (frame.size() < 2) {
        return; // 帧不完整
    }

    unsigned char opcode = frame[0] & 0x0F; // 提取操作码
    bool is_masked = (frame[1] & 0x80) != 0; // 检查是否有掩码
    size_t payload_len = frame[1] & 0x7F; // 提取负载长度

    size_t offset = 2;
    if (payload_len == 126) {
        // 扩展长度（2字节）
        payload_len = (frame[2] << 8) | frame[3];
        offset += 2;
    } else if (payload_len == 127) {
        // 扩展长度（8字节）
        payload_len = (frame[2] << 56) | (frame[3] << 48) | (frame[4] << 40) | (frame[5] << 32) |
                      (frame[6] << 24) | (frame[7] << 16) | (frame[8] << 8) | frame[9];
        offset += 8;
    }

    // 提取掩码键（如果有）
    unsigned char mask_key[4] = {0};
    if (is_masked) {
        for (int i = 0; i < 4; ++i) {
            mask_key[i] = frame[offset + i];
        }
        offset += 4;
    }

    // 提取负载数据
    std::string payload = frame.substr(offset, payload_len);

    // 如果数据被掩码，需要解掩码
    if (is_masked) {
        for (size_t i = 0; i < payload_len; ++i) {
            payload[i] ^= mask_key[i % 4];
        }
    }

    // 处理 WebSocket 消息
    if (opcode == 0x1) { // 文本帧
        handleWebSocketMessage(fd, payload);
    } else if (opcode == 0x8) { // 关闭帧
        LOG_INFO("Client " + std::to_string(fd) + " closed the connection.");
        if(online_members.find(fd) != online_members.end()){
            online_members.erase(fd);
        }
        close(fd);
    }
}

std::string compute_sec_websocket_accept(const char *sec_websocket_key) {
    // 创建一个临时缓冲区，用于拼接 Sec-WebSocket-Key 和 GUID
    char key_with_guid[256];
    static const char *ws_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    snprintf(key_with_guid, sizeof(key_with_guid), "%s%s", sec_websocket_key, ws_guid);

    // 计算 SHA-1 哈希
    unsigned char hash[20];
    SHA1::SHA1HashBytes((const unsigned char *)key_with_guid, strlen(key_with_guid), hash);
    //std::cout<<"SHA1 hash: "<<hash<<std::endl;
    // 编码为 Base64
    char encoded[100];  // Base64 编码后的字符串长度会增加
    Base64encode(encoded, (const char *)hash, 20);

    return std::string(encoded);  // 返回动态分配的字符串，调用者负责释放
}

void handleAuthRequest(int fd, const std::string& path, HttpMethod method, const std::string& body) {
    if (method == HttpMethod::OPTIONS) {
        handle_options_request(fd);
    } else if (path.find("/login") == 0) {
        user_login(fd, body);
    } else if (path.find("/register") == 0) {
        user_register(fd, body);
    }
}
void handleMessageRequest(int fd,HttpMethod method, const std::string& body) {
    if (method == HttpMethod::OPTIONS) {
        handle_options_request(fd);
        return;
    }
    MessageDAO msgDao;
    std::vector<Message> msg=msgDao.getpastMessages(fd);
    if(msg.empty()){
        LOG_ERROR("No message found");
        return;
    }
    std::string messageJson="{\"messages\": [";
    for(auto &m:msg){
        
        std::string message=m.MsgtoJsonManual();
        messageJson+=message+",";
    }
    messageJson=messageJson.substr(0,messageJson.size()-1);
    messageJson+="]}";
    HttpParser::sendHttpResponse(fd,HttpStatus::OK , {}, messageJson);
}

void onHttp_request(int fd, std::string message) {
    ClientContext &ctx = client_contexts[fd];
    size_t end_header = message.find("\r\n\r\n");
    
    if (end_header != std::string::npos) {
        std::string header = message.substr(0, end_header);
        std::string body = message.substr(end_header + 4);

        HttpRequest request = HttpParser::parseHttpRequest(message);
        std::string path = request.path;
        HttpMethod method = request.method;

        if (request.headers["Upgrade"] == "websocket")
        {
            std::cout << "Upgrade request" << std::endl;
            // WebSocket 升级请求
            std::string key = request.headers["Sec-WebSocket-Key"];
            std::string sec_websocket_accept = compute_sec_websocket_accept(key.c_str());
            if (sec_websocket_accept.empty())
            {
                std::cerr << "Failed to compute Sec-WebSocket-Accept" << std::endl;
                close(fd);
                return;
            }

            std::map<std::string, std::string> headers = {
                {"Upgrade", "websocket"},
                {"Connection", "Upgrade"},
                {"Sec-WebSocket-Accept", sec_websocket_accept},
            };
            std::string response = HttpParser::createHttpRequestResponse_1(HttpStatus::SWITCHING_PROTOCOLS, sec_websocket_accept, "");

            if (send(fd, response.c_str(), response.size(), 0) == -1)
            {
                std::cerr << "Send failed: " << strerror(errno) << std::endl;
                close(fd);
                return;
            }
            ctx.is_websocket = true;
            online_members.insert(fd);
            LOG_INFO("WebSocket connection established");
        }
        else if (path == "/register" || path == "/login")
        {
            handleAuthRequest(fd, path, method, body);
        }
        else if (path == "/getmessage")
        {
            handleMessageRequest(fd, method, body);
        }
        else
        {
            HttpParser::sendHttpResponse(fd, HttpStatus::OK, {}, "Unauthorized");
        }

        if (request.headers["Connection"] != "keep-alive" && request.headers["Connection"] != "keep-alive, Upgrade")
        {
            close(fd); // 非保持连接，直接关闭 fd
        }
        else
        {
            server.modFd(fd, EPOLLIN | EPOLLET); // 保持连接，重置 epoll 监听事件
        }
    }
}




void handleClientRead(int fd, uint32_t events)
{
    if (client_contexts.find(fd) == client_contexts.end())
    {
        client_contexts[fd] = ClientContext();
    }

    ClientContext &ctx = client_contexts[fd];
    char chunk[4096];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, chunk, sizeof(chunk))) > 0)
    {
        ctx.buffer.append(chunk, bytesRead); // 追加数据到缓冲区
        
        while (!ctx.buffer.empty())
        {
            if (!ctx.is_websocket)
            {
                // 处理 HTTP 请求
                size_t end_header = ctx.buffer.find("\r\n\r\n");
                if (end_header == std::string::npos)
                {
                    break; // 还没接收完整 HTTP 头部
                }

                std::string header = ctx.buffer.substr(0, end_header + 4);
                size_t content_length_pos = header.find("Content-Length: ");
                size_t content_length = 0;
                if (content_length_pos != std::string::npos)
                {
                    content_length = std::stoi(header.substr(content_length_pos + 16));
                }

                size_t total_size = end_header + 4 + content_length;
                if (ctx.buffer.size() < total_size)
                {
                    break; // HTTP 请求数据还未完全到达
                }

                std::string full_request = ctx.buffer.substr(0, total_size);
                ctx.buffer.erase(0, total_size); // 清理已处理部分
                
                ThreadPool::instance().commit([fd, full_request]() {
                    onHttp_request(fd, full_request);
                });
            }
            else
            {
                std::string Message_tmp=ctx.buffer;
                ctx.buffer.clear();
                ThreadPool::instance().commit([=]() {
                    handleWebSocketFrame(fd, Message_tmp);
                    
                });
                
                break;
            }
        }
    }

    if (bytesRead == 0)
    {
        LOG_INFO("Client FD:"+std::to_string(fd)+"disconnected.");
        //std::cout << "Client disconnected." << std::endl;
        //std::cout << "FD close!!" << fd << std::endl;
        client_contexts.erase(fd);
        if(online_members.find(fd) != online_members.end()){
            online_members.erase(fd);
        }
        close(fd);
    }
  
    else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        LOG_ERROR("Read error: " + std::string(strerror(errno)));
        //std::cerr << "Read error: " << strerror(errno) << std::endl;
        if(online_members.find(fd) != online_members.end()){
            online_members.erase(fd);
        }
        client_contexts.erase(fd);
        close(fd);
    }
}

// 处理新连接
void handleNewConnection(int fd, uint32_t events) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd == -1) {
        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        return;
    }

    //std::cout << "New connection from: " << inet_ntoa(client_addr.sin_addr) << std::endl;
    LOG_INFO("New connection from: " + std::string(inet_ntoa(client_addr.sin_addr)));
    LOG_INFO("FD: " + std::to_string(client_fd));
    // 设置客户端连接为非阻塞
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    // 注册客户端文件描述符的读事件,加入epoll反应堆
    server.addFd(client_fd, EPOLLIN | EPOLLET, handleClientRead);
}

// 处理标准输入,用于控制服务器
void handleStdin(int fd, uint32_t events) {
    char buffer[512];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::cout << "Received from stdin: " << buffer << std::endl;

        // 如果输入 "exit"，则关闭服务器
        if (strncmp(buffer, "exit", 4) == 0) {
            std::cout << "Exiting server..." << std::endl;
            exit(0);
        }
    }
}

int main() {
    if (!server.init()) {
        return 1;
    }

    // 创建监听套接字
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        LOG_ERROR("Socket创建失败");
        //std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        return 1;
    }

    // 设置 SO_REUSEADDR 选项，允许重用本地地址和端口
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        LOG_ERROR("设置 SO_REUSEADDR 失败!");
        close(server_fd);
        return 0;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); 
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定和监听
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        LOG_ERROR("绑定失败!");
        //std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        LOG_ERROR("监听失败!");
        //std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        return 1;
    }
    LOG_INFO("Server listening on port:");
    //std::cout << "Server listening on port 12345" << std::endl;

    // 设置服务器套接字为非阻塞
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    // 注册服务器套接字的可读事件（用于接受新的连接）
    server.addFd(server_fd, EPOLLIN, handleNewConnection);

    // 注册标准输入的事件（用于控制服务器的退出）
    server.addFd(STDIN_FILENO, EPOLLIN, handleStdin);

    // 运行事件循环
    server.run();

    close(server_fd);
    return 0;
}

#include "server.h"
#include "EpollReactor.h"
#include "http_header.h"
#include "base64.h"
#include "sha1.h"
#include "logger.h"
#include <set>
#include "global.h"
#include "router.h"
#include "http_handler.h"
#include "message.h"
#include "http_header.h"
#include "config.h"
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

Router router;

void initRoutes() {
    // HTTP 路由注册
    router.registerHttpRoute("POST", "/login", handleLogin);
    router.registerHttpRoute("POST", "/register", handleRegister);
    router.registerHttpRoute("POST", "/getmessage", handleGetMessage);
    router.registerHttpRoute("GET", "/", handleWebSocketUpgrade);  // 升级请求
    router.registerHttpRoute("OPTIONS", "*", handleOptions);  // OPTIONS 可以不区分 path

    // WebSocket 消息路由注册，根据 JSON 中 type 字段
    router.registerMessageRoute("1", handleBroadcastMessage);//广播消息 
    router.registerMessageRoute("2", handlePrivateMessage);//私聊
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
        LOG_DEBUG("received text frame: "+payload);
        if (!router.dispatchMessage(fd, payload)) {
            LOG_ERROR("No handler for WebSocket message: " + frame);
        }
    } else if (opcode == 0x8) { // 关闭帧
        LOG_INFO("Client " + std::to_string(fd) + " closed the connection.");
        if(online_members.find(fd) != online_members.end()){
            online_members.erase(fd);
        }
        close(fd);
    }
}

void onHttp_request(int fd, const std::string &message) {
    HttpRequest request = HttpParser::parseHttpRequest(message);
    if (request.method != HttpMethod::UNKNOWN) {
        if (!router.dispatchHttp(fd, request)) {
            HttpParser::sendHttpResponse(fd, HttpStatus::NOT_FOUND, {}, "Not Found");
        }
        // 判断是否保持连接
        if (request.headers["Connection"] != "keep-alive" &&
            request.headers["Connection"] != "keep-alive, Upgrade") {
            close(fd);
        } else {
            // 假设 server 为全局 EpollReactor 实例
            server.modFd(fd, EPOLLIN | EPOLLET);
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
        LOG_DEBUG("Received " + std::to_string(bytesRead) + " bytes from client " + std::to_string(fd));
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
        client_contexts.erase(fd);
        if(online_members.find(fd) != online_members.end()){
            online_members.erase(fd);
        }
        close(fd);
    }
  
    else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        LOG_ERROR("Read error: " + std::string(strerror(errno)));
        
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

    if (!Config::getInstance().load("config.ini")) {
        LOG_ERROR("Failed to load configuration");
        return 1;
    }

    initRoutes();
    if (!server.init()) {
        return 1;
    }

    ThreadPool::instance(std::thread::hardware_concurrency() * 20 );

    // 创建监听套接字
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        LOG_ERROR("Socket创建失败");
        return 1;
    }

    // 设置 SO_REUSEADDR 选项，允许重用本地地址和端口
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        LOG_ERROR("设置 SO_REUSEADDR 失败!");
        close(server_fd);
        return 0;
    }
    //获取发口号
    int PORT = Config::getInstance().getInt("server", "port", 12345);
    // 设置服务器地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); 
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定和监听
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        LOG_ERROR("绑定失败!");
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        LOG_ERROR("监听失败!");
        return 1;
    }
    LOG_INFO("Server listening on port:");

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

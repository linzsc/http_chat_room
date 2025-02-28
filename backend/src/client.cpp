
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include "http_header.h"
#include "message.h"
#define PORT 12345
// 客户端名称
std::string client_name = "linz1";

bool is_websocket = false;

// 发送消息函数
// 解析HTTP响应



// 接收消息函数
void receive_data(int clientSocket) {
    char buffer[4096];
    std::string total_buffer;  // 用于存储所有接收到的数据
    bool is_websocket = false; // 标记是否已经切换到 WebSocket 模式

    while (true) {
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            if (bytesRead == 0) {
                std::cout << "服务器断开连接!" << std::endl;
            } else {
                std::cerr << "接收数据失败: " << strerror(errno) << std::endl;
            }
            break;
        }

        buffer[bytesRead] = '\0';  // 确保字符串以 null 结尾
        total_buffer.append(buffer);  // 将接收到的数据追加到总缓冲区

        // 如果还没有进入 WebSocket 模式，解析 HTTP 响应
        if (!is_websocket) {
            size_t end_header = total_buffer.find("\r\n\r\n");
            if (end_header == std::string::npos) {
                std::cout << "HTTP 响应头部未完整接收" << std::endl;
                continue;  // 等待更多数据
            }

            std::string header = total_buffer.substr(0, end_header + 4);
            size_t content_length_pos = header.find("Content-Length: ");
            size_t content_length = 0;
            if (content_length_pos != std::string::npos) {
                content_length = std::stoi(header.substr(content_length_pos + 16));
            }

            size_t total_size = end_header + 4 + content_length;
            if (total_buffer.size() < total_size) {
                std::cout << "HTTP 响应数据未完整接收" << std::endl;
                continue;  // 等待更多数据
            }

            std::string full_response = total_buffer.substr(0, total_size);
            total_buffer.erase(0, total_size);  // 清理已处理部分

            // 解析 HTTP 响应
            HttpResponse res = HttpParser::parseHttpResponse(full_response);

            // 处理 WebSocket 升级
            if (res.status == HttpStatus::OK) {
                if (res.body[0]=='W') {
                    std::cout << "WebSocket连接已建立!" << std::endl;
                    is_websocket = true;  // 切换到 WebSocket 模式
                } else {
                    std::cout << "HTTP请求成功!" << std::endl;
                }
            } else {
                std::cout << "HTTP请求失败: " << static_cast<int>(res.status) << std::endl;
            }
        } else {

            std::cout<<"进入WebSocket模式"<<std::endl;
            // WebSocket 模式
            while (!total_buffer.empty()) {
                size_t begin_pos = total_buffer.find("\r\n\r\n");
                if (begin_pos == std::string::npos) {
                    break;  // WebSocket 消息未完整
                }

                begin_pos += 4;  // 跳过头部的 \r\n\r\n
                size_t end_pos = total_buffer.find("\r\n\r\n", begin_pos);
                if (end_pos == std::string::npos) {
                    break;  // WebSocket 消息未完整
                }

                std::string ws_message = total_buffer.substr(begin_pos, end_pos - 4);
                total_buffer.erase(0, end_pos + 4);  // 清理已处理部分

                // 解析 WebSocket 消息
                Message msg = Message::deserialize(ws_message);
                if (msg.get_type() == MessageType::GroupChat) {
                    std::cout << "[groupchat Sender:<" << msg.get_sender() << ">]: " << msg.get_content() << std::endl;
                } else if (msg.get_type() == MessageType::PrivateChat) {
                    std::cout << "[Sender:<" << msg.get_sender() << ">]: " << msg.get_content() << std::endl;
                }

                std::cout << "Message Timestamp:" << msg.get_timestamp() << std::endl;
            }
        }
    }

    close(clientSocket);
}
void sendHttpRequest(int cfd, const std::string& method, const std::string& path, const std::string& body) {
    std::ostringstream oss;
    oss << method << " " << path << " HTTP/1.1\r\n";
    oss << "Content-Type: application/json\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "\r\n";
    oss << body;

    std::string request = oss.str();
    std::cout << "发送HTTP请求: " << request << std::endl;
    send(cfd, request.c_str(), request.size(), 0);
}

// 发送消息函数
void send_data(int clientSocket) {
    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "exit") {
            break;
        }
        Message msg(MessageType::GroupChat, client_name, "linz1", message);
        std::string complete_message = msg.serialize();
        std::cout << "发送消息: " << complete_message << std::endl;
        send(clientSocket, complete_message.c_str(), complete_message.size(), 0);
    }
}

void user_regist(int cfd) {
    std::string username = "linz1";
    std::string password = "0128";
    std::string json_body = "{\"username\":\"" + username + "\", \"password\":\"" + password + "\"}";

    // 构造 HTTP 注册请求
    sendHttpRequest(cfd, "POST", "/register", json_body);
}

void user_login(int cfd) {
    std::string username = "linz1";
    std::string password = "0128";
    std::string json_body = "{\"username\":\"" + username + "\", \"password\":\"" + password + "\"}";

    // 构造 HTTP 登录请求
    sendHttpRequest(cfd, "POST", "/login", json_body);
}

void user_upgrate(int cfd) {
    sendHttpRequest(cfd, "POST", "/ws", "11111");
}
    // 构造 HTTP 登录请求

// 客户端启动函数
void start_client() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Socket 创建失败!" << std::endl;
        return;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(PORT);

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        std::cerr << "连接失败!" << std::endl;
        close(clientSocket);
        return;
    }

    std::cout << "成功连接到服务器!" << std::endl;

    std::thread recvThread(receive_data, clientSocket);
    std::thread sendThread(send_data, clientSocket);

    user_regist(clientSocket);
    sleep(1);
    user_login(clientSocket);
    sleep(1);
    user_upgrate(clientSocket);

    sendThread.join();
    recvThread.detach();

    close(clientSocket);
}

int main() {
    start_client();
    return 0;
}
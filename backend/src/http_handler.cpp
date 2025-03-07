#include "http_handler.h"
#include "logger.h"
#include "database.h"
#include "base64.h"
#include "sha1.h"
#include "simple_json.h"
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include "global.h"

std::map<HttpStatus, std::string> status_phrase_map = {
    {HttpStatus::OK, "OK"},
    {HttpStatus::NOT_FOUND, "Not Found"},
    {HttpStatus::SWITCHING_PROTOCOLS, "Switching Protocols"}
};

// 登录处理逻辑
void handleLogin(int fd, const HttpRequest &request) {
    SimpleJSONParser parser(request.body);
    auto user_info = parser.parse();
    std::string username = user_info["username"];
    std::string password = user_info["password"];

    LOG_INFO("Login request: " + username);
    UserDAO userDAO;
    int userId = userDAO.verifyLogin(username, password);
    if (userId != -1) {
        LOG_INFO("Login successful for user: " + username);
        std::map<std::string, std::string> headers = {{"Content-Type", "application/json"}};
        HttpParser::sendHttpResponse(fd, HttpStatus::OK, headers,
            "{\"success\": true, \"message\": \"Login successful\", \"userId\": \"" + std::to_string(userId) + "\"}");
    } else {
        LOG_ERROR("Login failed for user: " + username);
        std::map<std::string, std::string> headers = {{"Content-Type", "text/html"}};
        HttpParser::sendHttpResponse(fd, HttpStatus::NOT_FOUND, headers, "Unauthorized");
    }
}

// 注册处理逻辑
void handleRegister(int fd, const HttpRequest &request) {
    SimpleJSONParser parser(request.body);
    auto user_info = parser.parse();
    std::string username = user_info["username"];
    std::string password = user_info["password"];

    LOG_INFO("Register request: " + username);
    UserDAO userDAO;
    bool success = userDAO.registerUser(username, password);
    if (success) {
        LOG_INFO("Registration successful for user: " + username);
        std::map<std::string, std::string> headers = {{"Content-Type", "application/json"}};
        HttpParser::sendHttpResponse(fd, HttpStatus::OK, headers,
            "{\"success\": true, \"message\": \"Regist successful\"}");
    } else {
        LOG_ERROR("Registration failed for user: " + username);
        std::map<std::string, std::string> headers = {{"Content-Type", "application/json"}};
        HttpParser::sendHttpResponse(fd, HttpStatus::NOT_FOUND, headers,
            "{\"status\":\"这个id已经注册过了！\"}");
    }
}

// 获取历史消息处理逻辑
void handleGetMessage(int fd, const HttpRequest &request) {
    MessageDAO msgDao;
    auto messages = msgDao.getpastMessages(fd);
    if (messages.empty()) {
        LOG_ERROR("No message found for fd: " + std::to_string(fd));
        return;
    }
    std::string json = "{\"messages\": [";
    for (const auto &m : messages) {
        json += m.MsgtoJsonManual() + ",";
    }
    if (!messages.empty()) {
        json.pop_back();
    }
    json += "]}";
    HttpParser::sendHttpResponse(fd, HttpStatus::OK, {}, json);
}

// WebSocket 升级处理逻辑
void handleWebSocketUpgrade(int fd, const HttpRequest &request) {
    auto it = request.headers.find("Sec-WebSocket-Key");
    if (it == request.headers.end()) {
        LOG_ERROR("Sec-WebSocket-Key not found in upgrade request");
        close(fd);
        return;
    }
    std::string sec_key = it->second;
    char key_with_guid[256];
    static const char *ws_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    snprintf(key_with_guid, sizeof(key_with_guid), "%s%s", sec_key.c_str(), ws_guid);
    unsigned char hash[20];
    SHA1::SHA1HashBytes((const unsigned char*)key_with_guid, strlen(key_with_guid), hash);
    char encoded[100];
    Base64encode(encoded, (const char*)hash, 20);
    std::string sec_accept(encoded);

    std::string response = HttpParser::createHttpRequestResponse_1(HttpStatus::SWITCHING_PROTOCOLS, sec_accept, "");
    if (send(fd, response.c_str(), response.size(), 0) == -1) {
        LOG_ERROR("Failed to send WebSocket upgrade response");
        close(fd);
        return;
    }
    // 标记为 WebSocket 连接并加入在线成员
    client_contexts[fd].is_websocket = true;
    online_members.insert(fd);
    LOG_INFO("WebSocket connection established on fd: " + std::to_string(fd));
}

// OPTIONS 请求处理逻辑
void handleOptions(int fd, const HttpRequest &request) {
    std::string response =
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
        "Access-Control-Allow-Headers: *\r\n"
        "Content-Length: 0\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    send(fd, response.c_str(), response.size(), 0);
}
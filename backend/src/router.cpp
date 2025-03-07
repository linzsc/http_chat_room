#include "router.h"
#include "logger.h"
#include "simple_json.h"
#include <sstream>

// 生成路由键，格式为 "METHOD PATH"
std::string Router::makeHttpKey(const std::string &method, const std::string &path) const {
    return method + " " + path;
}

void Router::registerHttpRoute(const std::string &method, const std::string &path, HttpHandler handler) {
    httpRoutes[makeHttpKey(method, path)] = handler;
}

void Router::registerMessageRoute(const std::string &msgType, MessageHandler handler) {
    messageRoutes[msgType] = handler;
}

bool Router::dispatchHttp(int fd, const HttpRequest &request) {
    // 假设 HttpRequest 中有一个方法将枚举类型转换为字符串，比如 "GET", "POST"
    std::string methodStr;
    switch (request.method) {
        case HttpMethod::GET:    methodStr = "GET"; break;
        case HttpMethod::POST:   methodStr = "POST"; break;
        case HttpMethod::OPTIONS:methodStr = "OPTIONS"; break;
        default: methodStr = "UNKNOWN"; break;
    }
    LOG_DEBUG("Dispatching HTTP request: "+std::to_string(fd)+" "+methodStr+" "+request.path);
    std::string key = makeHttpKey(methodStr, request.path);
    auto it = httpRoutes.find(key);
    if (it != httpRoutes.end()) {
        it->second(fd, request);
        return true;
    }
    
    std::string wildcardKey = makeHttpKey(methodStr, "*");
    auto wildcardIt = httpRoutes.find(wildcardKey);
    if (wildcardIt != httpRoutes.end()) {
        wildcardIt->second(fd, request);
        return true;
    }
    
    LOG_ERROR("HTTP route not found for key: " + key);
    return false;
}

bool Router::dispatchMessage(int fd, const std::string &message) {
    // 使用简单 JSON 解析器解析消息，要求消息包含 type 字段
    SimpleJSONParser parser(message);
    std::map<std::string, std::string> msg_map = parser.parse();
    auto it = msg_map.find("type");
    if (it != msg_map.end()) {
        std::string msgType = it->second;
        auto handlerIt = messageRoutes.find(msgType);
        if (handlerIt != messageRoutes.end()) {
            handlerIt->second(fd, message);
            return true;
        } else {
            LOG_ERROR("Message route not found for type: " + msgType);
            return false;
        }
    }
    LOG_ERROR("Message type not specified in message: " + message);
    return false;
}
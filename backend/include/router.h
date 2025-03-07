
#ifndef ROUTER_H
#define ROUTER_H
#include "global.h"  // 定义全局变量
#include "http_header.h"  // 定义 HttpRequest、HttpMethod、HttpStatus 等
#include <functional>
#include <map>
#include <string>

// 路由模块同时支持 HTTP 和 WebSocket 消息
class Router {
public:
    // HTTP 请求处理函数签名：fd 和 HttpRequest
    using HttpHandler = std::function<void(int, const HttpRequest&)>;
    // WebSocket 消息处理函数签名：fd 和消息字符串
    using MessageHandler = std::function<void(int, const std::string&)>;

    // 注册 HTTP 路由，method 为 "GET"、"POST" 等，path 为请求路径
    void registerHttpRoute(const std::string &method, const std::string &path, HttpHandler handler);

    // 注册 WebSocket 消息路由，根据消息中的 type 字段（如 "broadcast"、"private" 等）
    void registerMessageRoute(const std::string &msgType, MessageHandler handler);

    // 根据 HttpRequest 分发请求，返回 true 表示找到了对应路由，否则 false
    bool dispatchHttp(int fd, const HttpRequest &request);

    // 根据消息内容分发消息，要求消息为 JSON 格式且包含 type 字段
    bool dispatchMessage(int fd, const std::string &message);

private:
    // 内部存储 HTTP 路由映射，键为 "METHOD PATH" 字符串，例如 "POST /login"
    std::map<std::string, HttpHandler> httpRoutes;
    // 内部存储消息路由映射，键为消息类型
    std::map<std::string, MessageHandler> messageRoutes;

    // 生成 HTTP 路由键
    std::string makeHttpKey(const std::string &method, const std::string &path) const;
};

#endif // ROUTER_H
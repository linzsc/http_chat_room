#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "http_header.h"

// 登录处理
void handleLogin(int fd, const HttpRequest &request);
// 注册处理
void handleRegister(int fd, const HttpRequest &request);
// 获取历史消息处理
void handleGetMessage(int fd, const HttpRequest &request);
// WebSocket 升级处理（GET 请求中包含 Sec-WebSocket-Key）
void handleWebSocketUpgrade(int fd, const HttpRequest &request);
// OPTIONS 请求处理
void handleOptions(int fd, const HttpRequest &request);

#endif // HTTP_HANDLER_H
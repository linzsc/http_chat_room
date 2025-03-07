#ifndef WS_HANDLER_H
#define WS_HANDLER_H

#include <string>

// 广播消息处理函数
void handleBroadcastMessage(int senderFd, const std::string &message);

// 私聊消息处理函数（示例暂未实现，默认使用广播）
void handlePrivateMessage(int senderFd, const std::string &message);

#endif // WS_HANDLER_H
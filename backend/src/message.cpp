#include "message.h"
#include "logger.h"
#include "simple_json.h"

#include "server.h"    // 定义 online_members
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include "global.h"

// 广播消息：解析消息、保存到数据库，然后向除发送者外的所有客户端广播
void handleBroadcastMessage(int senderFd, const std::string &message) {
    SimpleJSONParser parser(message);
    auto msg_map = parser.parse();
    MessageDAO messageDAO;
    if (messageDAO.saveMessage(msg_map["sender_id"], msg_map["sender_name"], msg_map["content"])) {
        LOG_INFO("Message saved to database successfully.");
    } else {
        LOG_ERROR("Failed to save message to database.");
    }
    // 构造简单 WebSocket 帧（仅支持文本帧，不做掩码处理）
    unsigned char frame[1024];
    size_t frame_len = 0;
    frame[frame_len++] = 0x81; // FIN=1, 文本帧
    size_t payload_len = message.size();
    if (payload_len < 126) {
        frame[frame_len++] = (unsigned char)payload_len;
    } else if (payload_len < 65536) {
        frame[frame_len++] = 126;
        frame[frame_len++] = (payload_len >> 8) & 0xFF;
        frame[frame_len++] = payload_len & 0xFF;
    } else {
        frame[frame_len++] = 127;
        for (int i = 0; i < 8; ++i) {
            frame[frame_len++] = (payload_len >> (56 - i * 8)) & 0xFF;
        }
    }
    memcpy(frame + frame_len, message.data(), message.size());
    frame_len += message.size();

    for (int client_fd : online_members) {
        if (client_fd != senderFd) {
            send(client_fd, frame, frame_len, 0);
            LOG_DEBUG("Broadcasting message to client: " + std::to_string(client_fd));
        }
    }
}

// 私聊消息处理（示例：未实现私聊逻辑，默认调用广播处理）
void handlePrivateMessage(int senderFd, const std::string &message) {
    LOG_INFO("Private message handler not implemented, defaulting to broadcast.");
    handleBroadcastMessage(senderFd, message);
}
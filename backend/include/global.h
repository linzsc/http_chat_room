#ifndef GLOBAL_H
#define GLOBAL_H

#include "EpollReactor.h"
#include <set>
#include <mutex>
struct ClientContext {
    std::string buffer;
    int buffer_pos;
    bool is_websocket; // 标识是否是WebSocket连接
    bool is_logged_in; // 标识是否已登录
    std::string ws_fragment_buffer;
    ClientContext() : buffer_pos(0), is_websocket(false), is_logged_in(false) {}
};
extern std::unordered_map<int, ClientContext> client_contexts;

extern  EpollReactor server;

extern std::set<int>online_members;
extern std::mutex online_mutex;
#endif // GLOBAL_H
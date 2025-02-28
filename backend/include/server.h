#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex>
#include <map>
#include "thread_poor.h"
#include "http_header.h"
#include "message.h"
#include "simple_json.h"
#include "database.h"
#define PORT 12345
#define BUFF_SIZE 1024

 
typedef void (*CallbackFunction)(int fd, uint32_t events);

// 在线用户列表

std::map<std::string, int>online_name;
std::map<int,std::string>online_fd;
std::map<std::string,std::string>Group_member;
std::unordered_map<std::string, std::string> user_db;


enum class ReadState {
    HEADER,  // 正在读取协议头
    BODY     // 正在读取消息体
};

/*
struct ClientContext {
    ReadState state = ReadState::HEADER;
    char buffer[4096];  // 临时缓冲区，用于累积接收的数据
    size_t buffer_pos = 0;  // 缓冲区当前存储位置
    ProtocolHeader header;  // 协议头
    char* body_buf = nullptr;  // 消息体缓冲区
    size_t body_len = 0;      // 当前消息体长度
    size_t body_capacity = 0; // 消息体缓冲区容量

    ~ClientContext() {
        // 释放消息体缓冲区
        if (body_buf) {
            free(body_buf);
        }
    }
};

*/

ThreadPool& thread_pool = ThreadPool::instance(std::thread::hardware_concurrency() * 20 );  // 使用线程池

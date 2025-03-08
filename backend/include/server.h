#pragma once
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
#define BUFF_SIZE 1024

 
typedef void (*CallbackFunction)(int fd, uint32_t events);




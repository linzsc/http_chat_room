# 指定 CMake 的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(chat_room_server)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加可执行文件
add_executable(server

    backend/src/global.cpp
    backend/src/config.cpp
    backend/src/server2.cpp
    backend/src/http_handler.cpp
    backend/src/router.cpp
    backend/src/message.cpp
    backend/src/sha1.cpp
    backend/src/base64.cpp
    backend/utils/logger.cpp
    backend/utils/database.cpp
)

# 指定头文件目录
target_include_directories(server PRIVATE backend/include)

target_link_libraries(server mysqlclient stdc++fs)
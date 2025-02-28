#include <iostream>
#include <thread>
#include <vector>
#include <mysql/mysql.h>
#include "database.h"  // 假设你的连接池类定义在这个头文件中

// 模拟一个简单的数据库操作
void simulateDatabaseOperation(MySQLConnPool& pool) {
    MYSQL* conn = pool.getConnection();
    if (conn) {
        std::cout << "Successfully obtained a connection." << std::endl;
        // 模拟数据库操作
        if (mysql_query(conn, "SELECT VERSION()")) {
            std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        } else {
            MYSQL_RES* result = mysql_store_result(conn);
            if (result) {
                std::cout << "MySQL Version: " << mysql_fetch_row(result)[0] << std::endl;
                mysql_free_result(result);
            }
        }
        pool.releaseConnection(conn);
    } else {
        std::cerr << "Failed to obtain a connection." << std::endl;
    }
}

int main() {
    // 获取连接池实例
    MySQLConnPool& pool = MySQLConnPool::getInstance();

    // 创建多个线程模拟并发访问
    const int num_threads = 10;
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(simulateDatabaseOperation, std::ref(pool));
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "All threads completed." << std::endl;
    return 0;
}
//g++ -o database test_connect.cpp . -I../include -lmysqlclient -lstdc++fs -std=c++11

#include "database.h"


bool UserDAO:: registerUser(const std::string &username, const std::string &password)
{
    MYSQL *conn = MySQLConnPool::getInstance().getConnection();
    std::string query = "INSERT INTO users (username, password_hash) VALUES (?, ?)";

    // 使用预处理语句防止SQL注入
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, query.c_str(), query.length());

    std::string hash = password; // 自行实现SHA-256哈希

    MYSQL_BIND params[2];
    memset(params, 0, sizeof(params));

    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (void *)username.c_str();
    params[0].buffer_length = username.length();

    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (void *)hash.c_str();
    params[1].buffer_length = hash.length();

    mysql_stmt_bind_param(stmt, params);
    bool success = (mysql_stmt_execute(stmt) == 0);

    mysql_stmt_close(stmt);
    MySQLConnPool::getInstance().releaseConnection(conn);
    return success;
}
/*
int UserDAO::verifyLogin(const std::string &username, const std::string &password)
{
    MYSQL *conn = MySQLConnPool::getInstance().getConnection();
    std::string query = "SELECT user_id FROM users WHERE username=? AND password_hash=?";

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, query.c_str(), query.length());

    std::string hash = password;
    MYSQL_BIND params[2] = {0};
    memset(params, 0, sizeof(params));

    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (void *)username.c_str();
    params[0].buffer_length = username.length();

    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (void *)hash.c_str();
    params[1].buffer_length = hash.length();

    mysql_stmt_bind_param(stmt, params);
    mysql_stmt_execute(stmt);

    MYSQL_BIND result;
    int user_id = -1;
    result.buffer_type = MYSQL_TYPE_LONG;
    result.buffer = &user_id;
    mysql_stmt_bind_result(stmt, &result);

    if (mysql_stmt_fetch(stmt) == 0)
    {
        user_id = *(int *)result.buffer;
    }

    mysql_stmt_close(stmt);
    MySQLConnPool::getInstance().releaseConnection(conn);
    return user_id;
}
*/

int UserDAO::verifyLogin(const std::string &username, const std::string &password) {
    MYSQL *conn = MySQLConnPool::getInstance().getConnection();
    if (!conn) {
        LOG_ERROR("获取数据库连接失败");
        return -1;
    }

    std::string query = "SELECT user_id FROM users WHERE username=? AND password_hash=?";
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        LOG_ERROR("初始化预处理语句失败: " + std::string(mysql_error(conn)));
        MySQLConnPool::getInstance().releaseConnection(conn);
        return -1;
    }

    // 1. 预处理语句
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        LOG_ERROR("预处理失败: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return -1;
    }

    // 2. 参数绑定
    std::string hash = password; // 需替换为实际哈希值
    MYSQL_BIND params[2] = {{0}}; // 显式初始化结构体
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (void*)username.c_str();
    params[0].buffer_length = username.length();

    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (void*)hash.c_str();
    params[1].buffer_length = hash.length();

    if (mysql_stmt_bind_param(stmt, params) != 0) {
        LOG_ERROR("绑定参数失败: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return -1;
    }

    // 3. 执行查询
    if (mysql_stmt_execute(stmt) != 0) {
        LOG_ERROR("执行失败: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return -1;
    }

    // 4. 关键修复：存储结果集到客户端
    if (mysql_stmt_store_result(stmt) != 0) {
        LOG_ERROR("无法存储结果集: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return -1;
    }

    // 5. 绑定结果
    int user_id = -1;
    MYSQL_BIND result = {0}; // 显式初始化
    result.buffer_type = MYSQL_TYPE_LONG;
    result.buffer = &user_id;
    if (mysql_stmt_bind_result(stmt, &result) != 0) {
        LOG_ERROR("绑定结果失败: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return -1;
    }

    // 6. 提取结果
    int fetch_status = mysql_stmt_fetch(stmt);
    if (fetch_status == MYSQL_NO_DATA) {
        LOG_INFO("用户不存在或密码错误");
    } else if (fetch_status != 0) {
        LOG_ERROR("提取结果失败: " + std::string(mysql_stmt_error(stmt)));
    }

    // 7. 释放结果集
    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    MySQLConnPool::getInstance().releaseConnection(conn);

    return user_id; // 成功时返回user_id，失败返回-1
}
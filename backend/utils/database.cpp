
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


bool MessageDAO::saveMessage(const std::string & sender_id, const std::string &sender,const std::string &content)
{   

    LOG_DEBUG("sender_id: "+sender_id+" sender: "+sender+" content: "+content);
    int receiverid = 1;
    std::string receiver="chat_room";

    int senderid=std::stoi(sender_id);

    MYSQL *conn = MySQLConnPool::getInstance().getConnection();
    if(!conn){
        LOG_ERROR("Get connection failed");
        return false;
    }
    std::string query =
        "INSERT INTO messages (sender_id, receiver_id, sender_name, receiver_name, content) VALUES (?, ?, ?, ?, ?)";

    MYSQL_STMT *stmt = mysql_stmt_init(conn);

    if (!stmt) {
        LOG_ERROR("prepare failed: " + std::string(mysql_error(conn)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return false;
    }

    if(mysql_stmt_prepare(stmt,query.c_str(), query.length())!=0){
        LOG_ERROR("prepare failed: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return false;
    }

    MYSQL_BIND params[5];
    memset(params, 0, sizeof(params));

    // 参数1: sender_id
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &senderid;

    // 参数2: receiver_id
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &receiverid;

    // 参数3: sender_name
    params[2].buffer_type = MYSQL_TYPE_STRING;
    params[2].buffer = (void *)sender.c_str();
    params[2].buffer_length = sender.length();

    // 参数4: receiver_name
    params[3].buffer_type = MYSQL_TYPE_STRING;
    params[3].buffer = (void *)receiver.c_str();
    params[3].buffer_length = receiver.length();

    // 参数5: content
    params[4].buffer_type = MYSQL_TYPE_STRING;
    params[4].buffer = (void *)content.c_str();
    params[4].buffer_length = content.length();

    if(mysql_stmt_bind_param(stmt, params)!=0){
        LOG_ERROR("Bind failed: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return -1;
    }

    if((mysql_stmt_execute(stmt) != 0)){
        LOG_ERROR("Execute failed: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
    }

    mysql_stmt_close(stmt);
    MySQLConnPool::getInstance().releaseConnection(conn);
    return true;
}

std::vector<Message> MessageDAO:: getpastMessages(int user_id) {
    MYSQL *conn = MySQLConnPool::getInstance().getConnection();
    std::vector<Message> messages;

    if (!conn) {
        LOG_ERROR("get connection failed");
        return messages;
    }

    std::string query =
        "SELECT message_id, sender_id, sender_name, content FROM messages WHERE receiver_id = ?"; // 修改查询以筛选特定用户

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        LOG_ERROR("初始化预处理语句失败: " + std::string(mysql_error(conn)));
        MySQLConnPool::getInstance().releaseConnection(conn);
        return messages;
    }
     // 预处理语句
     if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        LOG_ERROR("预处理失败: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return messages;
    }

    int tmpId=1;//修改为实际的用户ID
    // 绑定参数
    MYSQL_BIND param = {0};
    param.buffer_type = MYSQL_TYPE_LONG;
    param.buffer = &tmpId;

    if (mysql_stmt_bind_param(stmt, &param) != 0) {
        LOG_ERROR("绑定参数失败: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return messages;
    }

   

    // 执行查询
    if (mysql_stmt_execute(stmt) != 0) {
        LOG_ERROR("执行失败: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return messages;
    }

    // 存储结果集到客户端
    if (mysql_stmt_store_result(stmt) != 0) {
        LOG_ERROR("无法存储结果集: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return messages;
    }

    // 绑定结果集
    MYSQL_BIND results[4] = {0};
    int message_id, sender_id;
    char sender_name[255];
    char content[1024];
    unsigned long length[4];

    // 初始化绑定
    memset(results, 0, sizeof(results));

    results[0].buffer_type = MYSQL_TYPE_LONG;
    results[0].buffer = &message_id;

    results[1].buffer_type = MYSQL_TYPE_LONG;
    results[1].buffer = &sender_id;

    results[2].buffer_type = MYSQL_TYPE_STRING;
    results[2].buffer = sender_name;
    results[2].buffer_length = sizeof(sender_name);
    results[2].is_null = nullptr;
    results[2].length = &length[0];

    results[3].buffer_type = MYSQL_TYPE_STRING;
    results[3].buffer = content;
    results[3].buffer_length = sizeof(content);
    results[3].is_null = nullptr;
    results[3].length = &length[1];

    if (mysql_stmt_bind_result(stmt, results) != 0) {
        LOG_ERROR("绑定结果失败: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        MySQLConnPool::getInstance().releaseConnection(conn);
        return messages;
    }

    // 获取结果
    while (mysql_stmt_fetch(stmt) == 0) {
        messages.emplace_back(Message{
            message_id,
            sender_id,
            1,
            std::string(sender_name, length[0]),
            "chat_room",
            std::string(content, length[1])});
    }

    mysql_stmt_close(stmt);
    MySQLConnPool::getInstance().releaseConnection(conn);
    return messages;
}

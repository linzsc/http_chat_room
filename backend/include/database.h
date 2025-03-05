#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "logger.h"
#include "string.h"
#include <thread>
class MySQLConnPool{
public:
    static MySQLConnPool &getInstance()
    {
        static MySQLConnPool instance;
        return instance;
    }

    MYSQL *getConnection()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (connections_.empty() && current_size_ < max_size_)
        {
            MYSQL *conn = createNewConnection();
            if (conn)
            {
                ++current_size_;
                return conn;
            }
        }
        while (connections_.empty())
        {
            cond_.wait(lock);
        }
        MYSQL *conn = connections_.front();
        connections_.pop();
        lock.unlock();

        if (mysql_ping(conn) != 0)
        {
            LOG_ERROR("Connection lost, reconnecting...");
            conn = reconnect(conn);
            if (!conn)
            {
                LOG_ERROR("Reconnect failed.");
                return nullptr;
            }
            LOG_INFO("Reconnected successfully.");
        }
        return conn;
    }

    void releaseConnection(MYSQL *conn)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connections_.push(conn);
        cond_.notify_one();
    }

private:
    MySQLConnPool() : current_size_(0), max_size_(20), initial_size_(10)
    {
        for (int i = 0; i < initial_size_; ++i)
        {
            connections_.push(createNewConnection());
            ++current_size_;
        }
    }

    ~MySQLConnPool()
    {
        while (!connections_.empty())
        {
            MYSQL *conn = connections_.front();
            mysql_close(conn);
            connections_.pop();
        }
    }

    MYSQL *createNewConnection()
    {
        MYSQL *conn = mysql_init(nullptr);
        if (!mysql_real_connect(conn, "localhost", "chat_user", "linzsc@30",
                                "chat_system", 3306, nullptr, 0))
        {
            LOG_ERROR("Connection failed: " + std::string(mysql_error(conn)));
            mysql_close(conn);
            return nullptr;
        }
        return conn;
    }

    MYSQL *reconnect(MYSQL *conn)
    {
        for (int i = 0; i < 3; ++i)
        {
            mysql_close(conn);
            conn = mysql_init(nullptr);
            if (mysql_real_connect(conn, "localhost", "chat_user", "linzsc@30",
                                   "chat_system", 3306, nullptr, 0))
            {
                return conn;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return nullptr;
    }

    std::queue<MYSQL *> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
    int current_size_;
    const int max_size_;
    const int initial_size_;
};

    /*
class MySQLConnPool
{
public:
    static MySQLConnPool &getInstance()
    {
        static MySQLConnPool instance;
        return instance;
    }
   
    MYSQL *getConnection()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (connections_.empty())
        {
            cond_.wait(lock);
        }
        MYSQL *conn = connections_.front();
        connections_.pop();
        lock.unlock();

        // 检查连接是否有效
        if (mysql_ping(conn) != 0)
        {
            LOG_ERROR("Connection lost, reconnecting...");
            mysql_close(conn);
            conn = mysql_init(nullptr);
            if (!mysql_real_connect(conn, "localhost", "chat_user", "linzsc@30",
                                    "chat_system", 3306, nullptr, 0))
            {
                LOG_ERROR("Reconnect failed: " + std::string(mysql_error(conn)));
                mysql_close(conn);
                return nullptr; // 返回前考虑重试或处理错误
            }
            LOG_INFO("Reconnected successfully.");
        }
        return conn;
    }

    void releaseConnection(MYSQL *conn)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connections_.push(conn);
        cond_.notify_one();
    }

private:
    MySQLConnPool()
    {
        for (int i = 0; i < 10; ++i)
        {
            MYSQL *conn = mysql_init(nullptr);
            if (!mysql_real_connect(conn, "localhost", "chat_user", "linzsc@30",
                                    "chat_system", 3306, nullptr, 0))
            {
                // std::cerr << "Connection failed: " << mysql_error(conn) << std::endl;
                LOG_ERROR("Connection failed: " + std::string(mysql_error(conn)));
                // exit(1);
                exit(1);
            }
            connections_.push(conn);
        }
    }

    ~MySQLConnPool()
    {
        while (!connections_.empty())
        {
            MYSQL *conn = connections_.front();
            mysql_close(conn);
            connections_.pop();
        }
    }

    std::queue<MYSQL *> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
};
*/class UserDAO
{
public:
    bool registerUser(const std::string &username, const std::string &password); // 注册

    int verifyLogin(const std::string &username, const std::string &password); // 登陆

    // 修改密码
    // 添加好友
    // 删除好友
    // 获取好友列表
    // 获取好友信息
};

struct Message{
    int message_id;
    int sender_id;
    int receiver_id;
    std::string sender_name;
    std::string receiver_name;
    std::string content;
    //MYSQL_TIME sent_at;
    Message(int msg_id, int senderID, int receiverID, std::string senderName, std::string receiverName,const std::string& msg_content)
    : message_id(msg_id), sender_id(senderID), receiver_id(receiverID), sender_name(senderName), receiver_name(receiverName),content(msg_content) {}

    std::string MsgtoJsonManual() const {
        std::string json_str = "{";
        json_str += "\"message_id\":" + std::to_string(message_id) + ",";
        json_str += "\"sender_id\":" + std::to_string(sender_id) + ",";
        json_str += "\"receiver_id\":" + std::to_string(receiver_id) + ",";
        json_str += "\"sender_name\":\"" + escape(sender_name) + "\",";
        json_str += "\"receiver_name\":\"" + escape(receiver_name) + "\",";
        json_str += "\"content\":\"" + escape(content) + "\"";
        json_str += "}";
        return json_str;
    }
    
    std::string escape(const std::string& str) const {
        // 使用正则表达式或字符串替换处理特殊字符
        // 这里简单实现，仅处理双引号和反斜杠
        std::string result;
        for (char c : str) {
            if (c == '"') {
                result += "\\\"";
            } else if (c == '\\') {
                result += "\\\\";
            } else {
                result += c;
            }
        }
        return result;
    }
};

class MessageDAO
{
public:
    bool saveMessage(const std::string &sender_id, const std::string &sender, const std::string &content);//保存消息
    std::vector<Message> getpastMessages(int user_id);
    
};
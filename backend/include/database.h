    #include <mysql/mysql.h>
    #include <queue>
    #include <mutex>
    #include <condition_variable>
    #include "logger.h"
    #include "string.h"
    class MySQLConnPool {
    public:
        static MySQLConnPool& getInstance() {
            static MySQLConnPool instance;
            return instance;
        }
        /*
        MYSQL* getConnection() {
            std::unique_lock<std::mutex> lock(mutex_);
            while (connections_.empty()) {
                cond_.wait(lock);
            }
            MYSQL* conn = connections_.front();
            connections_.pop();
            return conn;
        }
        */
        MYSQL* getConnection() {
            std::unique_lock<std::mutex> lock(mutex_);
            while (connections_.empty()) {
                cond_.wait(lock);
            }
            MYSQL* conn = connections_.front();
            connections_.pop();
            lock.unlock();
        
            // 检查连接是否有效
            if (mysql_ping(conn) != 0) {
                LOG_ERROR("Connection lost, reconnecting...");
                mysql_close(conn);
                conn = mysql_init(nullptr);
                if (!mysql_real_connect(conn, "localhost", "chat_user", "linzsc@30",
                                        "chat_system", 3306, nullptr, 0)) {
                    LOG_ERROR("Reconnect failed: " + std::string(mysql_error(conn)));
                    mysql_close(conn);
                    return nullptr; // 返回前考虑重试或处理错误
                }
                LOG_INFO("Reconnected successfully.");
            }
            return conn;
        }

        void releaseConnection(MYSQL* conn) {
            std::unique_lock<std::mutex> lock(mutex_);
            connections_.push(conn);
            cond_.notify_one();
        }

    private:
        MySQLConnPool() {
            for (int i = 0; i < 10; ++i) {
                MYSQL* conn = mysql_init(nullptr);
                if (!mysql_real_connect(conn, "localhost", "chat_user", "linzsc@30",
                                        "chat_system", 3306, nullptr, 0)) {
                    //std::cerr << "Connection failed: " << mysql_error(conn) << std::endl;
                    LOG_ERROR("Connection failed: "+std::string(mysql_error(conn)));
                    //exit(1);
                    exit(1);
                }
                connections_.push(conn);
            }
        }

        ~MySQLConnPool() {
            while (!connections_.empty()) {
                MYSQL* conn = connections_.front();
                mysql_close(conn);
                connections_.pop();
            }
        }

        std::queue<MYSQL*> connections_;
        std::mutex mutex_;
        std::condition_variable cond_;
    };

    class UserDAO {
        public:
            bool registerUser(const std::string& username, const std::string& password);//注册
        
            int verifyLogin(const std::string& username, const std::string& password);//登陆

            //修改密码
            //添加好友
            //删除好友
            //获取好友列表
            //获取好友信息

        };
#include <iostream>
#include "database.h"
#include "logger.h"
int main() {

    //MySQLConnPool& pool = MySQLConnPool::getInstance();

    UserDAO userDAO;
    //随机生成用户名和密码
    std::string username="test_user_12",password="test_password";

    // 测试注册
    bool registerSuccess = userDAO.registerUser(username, password);
    LOG_INFO("User name:"+username+" password:"+password);
    if (registerSuccess) {
        std::cout << "User registered successfully." << std::endl;
    } else {
        std::cerr << "Failed to register user." << std::endl;
        //return 1;
    }
    
    // 测试登录
    int userId = userDAO.verifyLogin(username,password);
    if (userId != -1) {
        std::cout << "Login successful. User ID: " << userId << std::endl;
    } else {
        std::cerr << "Login failed." << std::endl;
        //return 1;
    }
    
    //测试获取消息
    MessageDAO messageDAO;
    std::vector<Message> messages = messageDAO.getpastMessages(1);

    for(auto &m: messages){
        std::string msg=m.MsgtoJsonManual();
        std::cout<<msg<<std::endl;
    }
    return 0;
}
//g++ -std=c++11 -o test_log_reg test_log_reg.cpp ../utils/database.cpp ../utils/logger.cpp -I ../include/ -lmysqlclient -lstdc++fs

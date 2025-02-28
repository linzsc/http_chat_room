#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

enum class MessageType {
    GroupChat = 1,  // 群聊消息
    PrivateChat,    // 私聊消息
    UserJoin,       // 用户加入
    UserExit        // 用户退出
};


struct Message {

    // 构造函数
    Message(){

    }

    Message(MessageType t,const std::string& s, const std::string& r, const std::string& c)
        : type(t), sender(s), receiver(r), content(c) {
        // 获取当前时间戳
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&time_t_now);
        std::ostringstream oss;
        oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
        timestamp = oss.str();
    }
    const std::string& get_sender() const {
        return sender;
    }
    const std::string& get_recv() const{
        return receiver;
    }
    const std::string& get_content() const {
        return content;
    }
    const std::string& get_timestamp() const {
        return timestamp;
    }
    const MessageType& get_type() const {
        return type;
    }
    // 序列化消息为字符串
    std::string serialize() const {
        std::ostringstream oss;
        oss << "\r\n\r\n"
            << static_cast<int>(type) << "|"
            << sender << "|"
            << receiver << "|"
            << content << "|"
            << timestamp
            <<"\r\n\r\n";
        return oss.str();
    }

    // 从字符串反序列化消息
    static Message deserialize(const std::string& data) {
        std::istringstream iss(data);
        int t;
        std::string s, r, c, type;
        //跳过前面的\r\n\r\n
        std::string temp;
        std::string Time;      // timestamp
        //std::getline(iss, temp, '\n'); // 跳过第一个 \r\n
        //std::getline(iss, temp, '\n'); // 跳过第二个 \r\n
        std::getline(iss, type, '|');  // type
        std::getline(iss, s, '|');  // sender
        std::getline(iss, r, '|');  // receiver
        std::getline(iss, c, '|'); // content
        std::getline(iss, Time);
        //std::getline(iss, temp, '\n'); // 跳过最后一个 \r\n

        t = std::stoi(type);
       
        std::cout << "type: " << type << std::endl;
        std::cout << "sender: " << s << std::endl;
        std::cout << "receiver: " << r << std::endl;
        std::cout << "content: " << c << std::endl;
        std::cout << "timestamp: " << Time << std::endl;

        return Message(static_cast<MessageType>(t), s, r, c); // 注意构造函数的参数顺序
    }

private:
    MessageType type;      // 消息类型
    std::string sender;    // 发送者用户名
    std::string receiver;  // 接收者用户名 ()
    std::string content;   // 消息内容
    std::string timestamp; // 时间戳
};

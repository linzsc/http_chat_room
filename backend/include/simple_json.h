#include <string>
#include <map>
#include <sstream>
#include <algorithm>

class SimpleJSONParser {
public:
    SimpleJSONParser(const std::string& json) : json_str(json) {}

    // 提取所有键值对，存储为 map
    std::map<std::string, std::string> parse() {
        std::map<std::string, std::string> result;
        size_t start = 0;
        size_t end = 0;

        // 跳过花括号
        start = json_str.find("{", start) + 1;
        end = json_str.find("}", end);

        while (start < end) {
            // 提取键
            size_t key_start = json_str.find("\"", start);
            if (key_start == std::string::npos) break;
            key_start++;
            size_t key_end = json_str.find("\"", key_start);
            if (key_end == std::string::npos) break;
            std::string key = json_str.substr(key_start, key_end - key_start);

            // 提取值
            size_t value_start = json_str.find("\"", key_end + 1);
            if (value_start == std::string::npos) break;
            value_start++;
            size_t value_end = json_str.find("\"", value_start);
            if (value_end == std::string::npos) break;
            std::string value = json_str.substr(value_start, value_end - value_start);

            result[key] = value;

            start = value_end + 1;
        }

        return result;
    }

private:
    std::string json_str;
};
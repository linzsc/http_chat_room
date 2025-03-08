#pragma once
#include <string>
#include <unordered_map>

class Config {
public:
    static Config& getInstance();
    
    bool load(const std::string& filename);
    
    // 获取配置项
    int getInt(const std::string& section, const std::string& key, int defaultVal = 0);
    std::string getStr(const std::string& section, const std::string& key, 
                     const std::string& defaultVal = "");
    bool getBool(const std::string& section, const std::string& key, bool defaultVal = false);
    
private:
    Config() = default;
    std::unordered_map<std::string, 
        std::unordered_map<std::string, std::string>> configData;
    
    void parseLine(const std::string& line, std::string& currentSection);
};
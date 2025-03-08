#include "config.h"
#include "logger.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::load(const std::string& filename) {
    std::string currentPath = std::filesystem::current_path();
    LOG_INFO("Current working directory: " + currentPath);
    std::ifstream file("../"+filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open config file: " + filename);
        return false;
    }

    std::string line;
    std::string currentSection;
    
    while (std::getline(file, line)) {
        // 去除首尾空白
        line.erase(line.find_last_not_of(" \t") + 1);
        line.erase(0, line.find_first_not_of(" \t"));
        
        // 处理空行和注释
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        parseLine(line, currentSection);
    }
    
    LOG_INFO("Config loaded successfully");
    return true;
}

void Config::parseLine(const std::string& line, std::string& currentSection) {
    // 处理节
    if (line.front() == '[' && line.back() == ']') {
        currentSection = line.substr(1, line.size() - 2);
        std::transform(currentSection.begin(), currentSection.end(),
                      currentSection.begin(), ::tolower);
        return;
    }

    // 处理键值对
    size_t eqPos = line.find('=');
    if (eqPos != std::string::npos) {
        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);
        
        // 去除键值对的空白
        key.erase(key.find_last_not_of(" \t") + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        
        value.erase(value.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        
        // 转换为小写保存节和键
        std::string lowerSection = currentSection;
        std::transform(lowerSection.begin(), lowerSection.end(),
                      lowerSection.begin(), ::tolower);
        
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(),
                      lowerKey.begin(), ::tolower);
        
        configData[lowerSection][lowerKey] = value;
    }
}

int Config::getInt(const std::string& section, const std::string& key, int defaultVal) {
    std::string s = section;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    
    auto itSection = configData.find(s);
    if (itSection != configData.end()) {
        auto itKey = itSection->second.find(k);
        if (itKey != itSection->second.end()) {
            try {
                return std::stoi(itKey->second);
            } catch (...) {
                LOG_ERROR("Invalid integer value for config: " + s + "." + k);
            }
        }
    }
    return defaultVal;
}

std::string Config::getStr(const std::string& section, const std::string& key,
                          const std::string& defaultVal) {
    std::string s = section;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    
    auto itSection = configData.find(s);
    if (itSection != configData.end()) {
        auto itKey = itSection->second.find(k);
        if (itKey != itSection->second.end()) {
            return itKey->second;
        }
    }
    return defaultVal;
}

bool Config::getBool(const std::string& section, const std::string& key, bool defaultVal) {
    std::string val = getStr(section, key, "");
    std::transform(val.begin(), val.end(), val.begin(), ::tolower);
    
    if (val == "true" || val == "1" || val == "yes") return true;
    if (val == "false" || val == "0" || val == "no") return false;
    return defaultVal;
}
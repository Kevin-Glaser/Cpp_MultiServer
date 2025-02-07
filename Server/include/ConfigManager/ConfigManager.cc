#include "ConfigManager.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <iostream>

std::string ConfigManager::readConfigByKey(const std::string& filename,
                                    const std::string& section,
                                    const std::string& key) {
    try {
        auto sectionConfig = readConfigBySection(filename, section);
        if (sectionConfig.count(key)) {
            return sectionConfig[key];
        }
        throw std::runtime_error("Key not found: [" + section + "] " + key);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to read config: " + std::string(e.what()));
    }
}

bool ConfigManager::writeConfig(const std::string& filename,
                              const std::string& section,
                              const std::string& key,
                              const std::string& value) {
    try {
        if (!makeBackup(filename)) {
            return false;
        }

        auto config = parseConfigFile(filename);
        config[section][key] = value;

        if (!writeConfigFile(filename, config)) {
            restoreFromBackup(filename);
            return false;
        }

        std::filesystem::remove(filename + ".bak");
        return true;
    } catch (const std::exception& e) {
        restoreFromBackup(filename);
        return false;
    }
}

bool ConfigManager::makeBackup(const std::string& filename) {
    try {
        if (std::filesystem::exists(filename)) {
            std::filesystem::copy_file(filename, filename + ".bak",
                std::filesystem::copy_options::overwrite_existing);
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool ConfigManager::restoreFromBackup(const std::string& filename) {
    try {
        if (std::filesystem::exists(filename + ".bak")) {
            std::filesystem::rename(filename + ".bak", filename);
        }
        return true;
    } catch (...) {
        return false;
    }
}

std::map<std::string, std::map<std::string, std::string>> 
ConfigManager::parseConfigFile(const std::string& filename) {
    std::map<std::string, std::map<std::string, std::string>> config;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string line, currentSection;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                currentSection = line.substr(1, end - 1);
            }
            continue;
        }

        size_t pos = line.find('=');
        if (pos != std::string::npos && !currentSection.empty()) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            config[currentSection][key] = value;
        }
    }

    return config;
}

bool ConfigManager::writeConfigFile(const std::string& filename,
    const std::map<std::string, std::map<std::string, std::string>>& config) {
    
    std::string tempFile = filename + ".tmp";
    std::ofstream file(tempFile);
    if (!file) {
        return false;
    }

    for (const auto& [section, values] : config) {
        file << "[" << section << "]\n";
        for (const auto& [key, value] : values) {
            file << key << "=" << value << "\n";
        }
        file << "\n";
    }

    file.close();
    if (file.fail()) {
        std::filesystem::remove(tempFile);
        return false;
    }

    try {
        std::filesystem::rename(tempFile, filename);
        return true;
    } catch (...) {
        std::filesystem::remove(tempFile);
        return false;
    }
}

// 读取整个配置文件
std::map<std::string, std::map<std::string, std::string>> 
ConfigManager::readAllConfig(const std::string& filename) {
    try {
        return parseConfigFile(filename);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to read config file: " + std::string(e.what()));
    }
}

// 读取指定section的所有配置
std::map<std::string, std::string> 
ConfigManager::readConfigBySection(const std::string& filename, const std::string& section) {
    try {
        auto config = parseConfigFile(filename);
        if (config.count(section)) {
            return config[section];
        }
        throw std::runtime_error("Section not found: [" + section + "]");
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to read section: " + std::string(e.what()));
    }
}

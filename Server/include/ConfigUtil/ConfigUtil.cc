#include "ConfigUtil.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <iostream>

namespace fs = std::filesystem;

std::filesystem::path ConfigManager::getConfigPath(const std::string& filename) {
    fs::path configPath;
#ifdef WIN32_PLATFORM
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    configPath = fs::path(buffer).parent_path() / filename;
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
    if (len != -1) {
        buffer[len] = '\0';
        configPath = fs::path(buffer).parent_path() / filename;
    } else {
        // 如果readlink失败，使用当前工作目录
        configPath = fs::current_path() / filename;
    }
#endif
    return configPath;
}

std::filesystem::path ConfigManager::getBackupPath(const std::string& filename) {
    return getConfigPath(filename).concat(".bak");
}

bool ConfigManager::isValidPath(const std::filesystem::path& path) {
    std::error_code ec;
    if (fs::exists(path, ec)) {
        return fs::is_regular_file(path, ec);
    }
    return true;  // 允许创建新文件
}

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
        fs::path srcPath = getConfigPath(filename);
        fs::path backupPath = getBackupPath(filename);
        
        if (fs::exists(srcPath)) {
            fs::copy_file(srcPath, backupPath, 
                fs::copy_options::overwrite_existing);
        }
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Backup failed: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::restoreFromBackup(const std::string& filename) {
    try {
        fs::path srcPath = getConfigPath(filename);
        fs::path backupPath = getBackupPath(filename);
        
        if (fs::exists(backupPath)) {
            fs::rename(backupPath, srcPath);
        }
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Restore failed: " << e.what() << std::endl;
        return false;
    }
}

std::map<std::string, std::map<std::string, std::string>> 
ConfigManager::parseConfigFile(const std::string& filename) {
    fs::path configPath = getConfigPath(filename);
    if (!isValidPath(configPath)) {
        throw std::runtime_error("Invalid config file path: " + configPath.string());
    }

    std::map<std::string, std::map<std::string, std::string>> config;
    std::ifstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + configPath.string());
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
    
    fs::path configPath = getConfigPath(filename);
    fs::path tempPath = configPath.concat(".tmp");

    if (!isValidPath(configPath)) {
        return false;
    }

    std::ofstream file(tempPath);
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
        std::filesystem::remove(tempPath);
        return false;
    }

    try {
        fs::rename(tempPath, configPath);
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Write failed: " << e.what() << std::endl;
        fs::remove(tempPath);
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

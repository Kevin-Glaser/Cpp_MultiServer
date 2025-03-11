/**
 * @file ConfigUtil.h
 * @author KevinGlaser
 * @brief  * In this component, we will implement a ConfigManager class to read and write configuration 
 *           files based on specific conditions.
 *           This component will use the filesystem library from C++17 instead of the older fstream, 
 *           as filesystem provides more comprehensive and safer functions
 * @version 0.1
 * @date 2025-03-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __CONFIGMANAGER_H__
#define __CONFIGMANAGER_H__

#include <string>
#include <map>
#include <filesystem>

#ifdef WIN32_PLATFORM
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
    #include <linux/limits.h>
    #ifndef PATH_MAX
        #define PATH_MAX 4096
    #endif
#endif

class ConfigManager {
public:
    /**
     * @brief Reads all sections and their key-value pairs from a configuration file
     * @param filename[in] Path to the configuration file
     * @return Map containing all sections with their respective key-value pairs
     */
    static std::map<std::string, std::map<std::string, std::string>> 
    readAllConfig(const std::string& filename);
    
    /**
     * @brief Reads all key-value pairs from a specific section in the configuration file
     * @param filename[in] Path to the configuration file
     * @param section[in] Name of the section to read
     * @return Map containing all key-value pairs in the specified section
     */
    static std::map<std::string, std::string> 
    readConfigBySection(const std::string& filename, const std::string& section);
    
    /**
     * @brief Reads a specific value from a section and key in the configuration file
     * @param filename[in] Path to the configuration file
     * @param section[in] Name of the section containing the key
     * @param key[in] Name of the key to read
     * @return String value associated with the specified key
     */
    static std::string readConfigByKey(const std::string& filename, 
                                const std::string& section,
                                const std::string& key);
    
    /**
     * @brief Writes a value to a specific section and key in the configuration file
     * @param filename[in] Path to the configuration file
     * @param section[in] Name of the section to write to
     * @param key[in] Name of the key to write
     * @param value[in] Value to write
     * @return True if write operation was successful, false otherwise
     */
    static bool writeConfig(const std::string& filename,
                          const std::string& section,
                          const std::string& key,
                          const std::string& value);

private:
    /**
     * @brief Creates a backup copy of the configuration file
     * @param filename[in] Path to the configuration file to backup
     * @return True if backup was successful, false otherwise
     */
    static bool makeBackup(const std::string& filename);

    /**
     * @brief Restores configuration file from its backup
     * @param filename[in] Path to the configuration file to restore
     * @return True if restoration was successful, false otherwise
     */
    static bool restoreFromBackup(const std::string& filename);

    /**
     * @brief Parses a configuration file into a structured map
     * @param filename[in] Path to the configuration file to parse
     * @return Map containing parsed configuration data
     */
    static std::map<std::string, std::map<std::string, std::string>> 
    parseConfigFile(const std::string& filename);

    /**
     * @brief Writes configuration data to a file
     * @param filename[in] Path to the configuration file to write
     * @param config[in] Configuration data to write
     * @return True if write operation was successful, false otherwise
     */
    static bool writeConfigFile(const std::string& filename,
                              const std::map<std::string, 
                              std::map<std::string, std::string>>& config);

    /**
     * @brief Get the Config Path object
     * @param filename 
     * @return * std::filesystem::path 
     */
    static std::filesystem::path getConfigPath(const std::string& filename);

    /**
     * @brief Get the Backup Path object
     * @param filename 
     * @return std::filesystem::path 
     */
    static std::filesystem::path getBackupPath(const std::string& filename);

    /**
     * @brief Check if the path is valid
     * 
     * @param path 
     * @return true equals to the path is valid
     * @return false equals to the path is invalid
     */
    static bool isValidPath(const std::filesystem::path& path);
};

#endif
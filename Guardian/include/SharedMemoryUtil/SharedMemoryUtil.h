#ifndef SHARED_MEMORY_UTIL_H
#define SHARED_MEMORY_UTIL_H

#include <iostream>
#include <string>
#include <mutex>
#include <fstream>
#include <ctime>
#include <cstring>

#include "JsonUtil/json.hpp"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
#endif

static std::mutex log_mutex;
using json = nlohmann::json;

static void writeLog(const std::string& message) {
    // 锁定互斥锁，保证同一时间只有一个线程可以进入临界区
    std::lock_guard<std::mutex> lock(log_mutex);

    std::ofstream log("./guardian.log", std::ios_base::app);
    if (log.is_open()) {
        // 获取当前时间
        time_t now = time(nullptr);
        struct tm timeinfo;
        #ifdef _WIN32
            localtime_s(&timeinfo, &now);
        #else
            localtime_r(&now, &timeinfo);
        #endif

        // 定义时间格式
        char buffer[20]; // 确保有足够的空间存储格式化后的时间字符串
        strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H:%M:%S", &timeinfo);

        // 写入日志文件
        log << buffer << " " << message << std::endl;
        log.close();
    } else {
        // 如果无法打开日志文件，直接输出到 stderr，并且这里不需要加锁因为这是一个错误路径
        std::cerr << "Unable to open log file: /home/demo/Documents/Cpp_MultiServer/Guardian/guardian.log" << std::endl;
    }
}

class SharedMemoryManager {
public:
    SharedMemoryManager(const char* name, const size_t size)
        : shm_name(name)
        , shm_size(size)
        #ifdef _WIN32
            , hMapFile(NULL)
            , shared_data(NULL)
        #else
            , shm_fd(-1)
            , shared_data(nullptr)
        #endif
    {
        writeLog("SharedMemoryManager created with name: " + std::string(name));
    }

    ~SharedMemoryManager() {
        cleanupSharedMemory();
    }

    bool createSharedMemory();
    void writeData(const std::string& jsonStr);
    void cleanupSharedMemory();

private:
    const char* shm_name;
    size_t shm_size;

    #ifdef _WIN32
        HANDLE hMapFile;
        LPVOID shared_data;
    #else
        int shm_fd;
        char* shared_data;
    #endif
};

#endif
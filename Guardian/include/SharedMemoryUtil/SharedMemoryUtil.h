#ifndef SHARED_MEMORY_UTIL_H
#define SHARED_MEMORY_UTIL_H

#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <ctime>
#include <stdexcept>
#include <sstream>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <mutex>
#include <signal.h>
#include <unordered_set>

#include "JsonUtil/json.hpp"

static std::mutex log_mutex;
using json = nlohmann::json;

static void writeLog(const std::string& message) {
    // 锁定互斥锁，保证同一时间只有一个线程可以进入临界区
    std::lock_guard<std::mutex> lock(log_mutex);

    std::ofstream log("/home/demo/Documents/Cpp_MultiServer/Guardian/guardian.log", std::ios_base::app);
    if (log.is_open()) {
        // 获取当前时间
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo); // 使用线程安全版本的 localtime

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
        : shm_name(name), shm_size(size), shm_fd(-1), shared_data(nullptr) {}

    ~SharedMemoryManager() {
        cleanupSharedMemory();
    }

    bool createSharedMemory();
    void writeData(const std::string& jsonStr);
    void cleanupSharedMemory();

private:
    const char* shm_name;
    size_t shm_size;
    int shm_fd;
    char* shared_data;
};

#endif
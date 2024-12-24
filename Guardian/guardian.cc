#ifndef SHARED_MEMORY_MANAGER_H
#define SHARED_MEMORY_MANAGER_H

#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <ctime>
#include <signal.h>
#include <sys/wait.h>
#include <thread>
#include <cstring>

const char* SHM_NAME = "/shared_memory_example";
const size_t SHM_SIZE = 4096;
const char* LOG_FILE = "/home/demo/Documents/Cpp_MultiServer/Guardian/guardian.log";
const char* SERVER_PATH = "/home/demo/Documents/Cpp_MultiServer/Guardian/server"; // 目标进程的路径

// 写日志
void writeLog(const std::string& message) {
    std::ofstream log(LOG_FILE, std::ios_base::app);
    if (log.is_open()) {
        time_t now = time(nullptr);
        char* dt = ctime(&now);
        log << dt << " " << message << std::endl;
        log.close();
    } else {
        std::cerr << "Unable to open log file: " << LOG_FILE << std::endl;
    }
}

class SharedMemoryManager {
public:
    SharedMemoryManager()
        : shm_name(SHM_NAME), shm_size(SHM_SIZE), shm_fd(-1), shared_data(nullptr), server_pid(0) {}

    ~SharedMemoryManager() {
        cleanupSharedMemory();
    }

    void start() {
        if (!createSharedMemory()) {
            return;
        }

        startServer();
        if (waitForStartupSuccess()) {
            writeLog("Guardian exiting after server startup success");
            exit(0); // 守护进程退出
        }
    }

private:
    bool createSharedMemory() {
        shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            writeLog("Failed to create shared memory: " + std::string(std::strerror(errno)));
            return false;
        }

        if (ftruncate(shm_fd, shm_size) == -1) {
            writeLog("Failed to set shared memory size: " + std::string(std::strerror(errno)));
            close(shm_fd);
            return false;
        }

        shared_data = (char*) mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shared_data == MAP_FAILED) {
            writeLog("Failed to map shared memory: " + std::string(std::strerror(errno)));
            close(shm_fd);
            return false;
        }

        return true;
    }

    void cleanupSharedMemory() {
        if (shared_data) {
            munmap(shared_data, shm_size);
            shared_data = nullptr;
        }
        if (shm_fd != -1) {
            close(shm_fd);
            shm_unlink(shm_name); // 删除共享内存对象
            shm_fd = -1;
        }
    }

    void startServer() {
        writeLog("Starting server...");
        server_pid = fork();
        if (server_pid == 0) {
            // 子进程
            execl(SERVER_PATH, "server_program", (char*)nullptr);
            std::cerr << "Failed to execute server program: " << std::strerror(errno) << std::endl;
            _exit(1); // 子进程退出
        } else if (server_pid > 0) {
            // 父进程
            writeLog("Server started with PID: " + std::to_string(server_pid));
        } else {
            // fork 失败
            writeLog("Failed to fork: " + std::string(std::strerror(errno)));
            exit(1);
        }
    }

    bool waitForStartupSuccess() {
        while (true) {
            std::string message(shared_data);
            if (message == "OnStartup Success") {
                writeLog(message);
                std::memset(shared_data, 0, shm_size); // 清空消息
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    const char* shm_name;
    size_t shm_size;
    int shm_fd;
    char* shared_data;
    pid_t server_pid;

    // 禁止复制构造函数和赋值操作符
    SharedMemoryManager(const SharedMemoryManager&) = delete;
    SharedMemoryManager& operator=(const SharedMemoryManager&) = delete;
};

#endif // SHARED_MEMORY_MANAGER_H

int main(int argc, char* argv[]) {
    auto guardian = std::make_shared<SharedMemoryManager>();
    guardian->start();
    return 0;
}
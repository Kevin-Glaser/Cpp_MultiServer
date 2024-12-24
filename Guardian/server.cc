#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <cstdlib>
#include <ctime>
#include <signal.h>
#include <thread>
#include <string.h> // 包含 string.h 头文件

const char* SHM_NAME = "/shared_memory_example";
const size_t SHM_SIZE = 4096;

class Server {
private:
    class Buffer {
    private:
        int shm_fd;
        char* data;
        const size_t size;
        const char* name;

    public:
        Buffer(const char* name, size_t size) : name(name), size(size), data(nullptr) {
            shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
            if (shm_fd == -1) {
                throw std::runtime_error("Failed to open shared memory: " + std::string(strerror(errno)));
            }

            if (ftruncate(shm_fd, size) == -1) {
                throw std::runtime_error("Failed to set shared memory size: " + std::string(strerror(errno)));
            }

            data = (char*) mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (data == MAP_FAILED) {
                throw std::runtime_error("Failed to map shared memory: " + std::string(strerror(errno)));
            }
        }

        ~Buffer() {
            if (data) {
                munmap(data, size);
            }
            if (shm_fd != -1) {
                close(shm_fd);
            }
        }

        void write(const std::string& message) {
            if (message.length() >= size) {
                throw std::length_error("Message length exceeds buffer size");
            }
            strncpy(data, message.c_str(), size);
        }

        std::string read() const {
            return std::string(data);
        }

        char* getData() const {
            return data;
        }
    };

    Buffer buffer;

    void sendMessageToGuardian(const std::string& message) {
        buffer.write(message);
    }

    static void handleSignal(int signum, siginfo_t* info, void* context) {
        Server* server = static_cast<Server*>(info->si_value.sival_ptr);
        server->sendMessageToGuardian("OnStopped Exception");
        std::cerr << "Server received signal: " << signum << std::endl;
        exit(1); // 异常退出
    }

    void registerSignalHandlers() {
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO; // 使用 SA_SIGINFO 标志
        sa.sa_sigaction = &Server::handleSignal;

        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);
    }

public:
    Server() : buffer(SHM_NAME, SHM_SIZE) {}

    void start() {
        // 注册信号处理函数
        registerSignalHandlers();

        // 初始化资源
        std::cout << "Initializing resources..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 模拟初始化过程
        sendMessageToGuardian("OnStartup Success");
        std::cout << "Server started successfully" << std::endl;

        // 模拟正常运行
        while (true) {
            std::string message = buffer.read();
            if (message == "OnStop") {
                std::cout << "Server received OnStop message" << std::endl;
                // 保存正在运行的内容
                std::cout << "Saving state..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2)); // 模拟保存过程
                sendMessageToGuardian("OnStopped Success");
                std::cout << "Server stopped successfully" << std::endl;
                return; // 正常退出
            }

            // 模拟异常情况
            if (rand() % 10 == 0) {
                sendMessageToGuardian("OnStopped Exception");
                std::cerr << "Server encountered an exception" << std::endl;
                exit(1); // 异常退出
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

int main() {
    srand(time(nullptr)); // 初始化随机数种子
    Server server;
    server.start();
    return 0;
}
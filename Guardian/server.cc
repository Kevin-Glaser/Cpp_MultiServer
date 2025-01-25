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
#include <cstring> // 包含 string.h 头文件
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>

#include "include/JsonUtil/json.hpp"

using json = nlohmann::json;

const char* SHM_NAME = "test_shm";
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
            // 尝试打开现有的共享内存对象
            shm_fd = shm_open(name, O_RDWR, 0666);
            if (shm_fd == -1) {
                throw std::runtime_error("Failed to open shared memory: " + std::string(strerror(errno)));
            }

            // 确认共享内存对象的大小
            struct stat sb;
            if (fstat(shm_fd, &sb) == -1) {
                throw std::runtime_error("Failed to get shared memory size: " + std::string(strerror(errno)));
            }
            if (sb.st_size < static_cast<off_t>(size)) {
                throw std::runtime_error("Shared memory object is too small");
            }

            // 映射共享内存到进程地址空间
            data = (char*) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (data == MAP_FAILED) {
                close(shm_fd);
                throw std::runtime_error("Failed to map shared memory: " + std::string(strerror(errno)));
            }

            std::cout << "Shared memory mapped successfully." << std::endl;
        }

        ~Buffer() {
            if (data != nullptr) {
                if (munmap(data, size) == -1) {
                    std::cerr << "Failed to unmap shared memory: " << strerror(errno) << std::endl;
                }
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
    int server_socket;
    unsigned short port; // 添加端口号变量

    // 添加新的成员变量
    static const int MAX_MISSED_HEARTBEATS = 3;  // 最大允许的连续未收到响应次数
    int missed_heartbeat_count;
    bool heartbeat_alive;
    std::time_t last_heartbeat_response;

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

    bool connectToPort(unsigned short port) {
        std::cout << port << std::endl;
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

        if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            std::cout << "Failed to connect to the specified port in connectToPort" << std::endl;
            close(server_socket);
            return false;
        }
        return true;
    }

    bool reconnectToPort(unsigned short port) {
        while (true) {
            std::cout << "Attempting to reconnect..." << std::endl;
            if (connectToPort(port)) {
                std::cout << "Reconnected successfully." << std::endl;
                return true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void sendMessage(const char* action, unsigned short port) {
        std::string message = "connected on port " + std::to_string(port);

        json j = {
            {"action", action},
            {"msg", message.c_str()}
        };
        std::string msg = j.dump();
        // 发送消息到守护进程或客户端
        const char* msg_to_send = msg.c_str();
        if (send(server_socket, msg_to_send, strlen(msg_to_send), 0) < 0) {
            throw std::runtime_error("Failed to send message: " + std::string(strerror(errno)));
        }
    }

    bool waitForStartupConfirmation() {
        char buffer[1024];
        // 等待守护进程的启动确认消息
        ssize_t numBytes = recv(server_socket, buffer, sizeof(buffer) - 1, 0);
        if (numBytes <= 0) {
            std::cerr << "Failed to receive startup confirmation" << std::endl;
            return false;
        }

        buffer[numBytes] = '\0';
        try {
            auto response = json::parse(buffer);
            if (response["action"] == "startup" && response["msg"] == "confirmed") {
                std::cout << "Received startup confirmation from guardian" << std::endl;
                return true;
            }
        } catch (const json::parse_error& e) {
            std::cerr << "Failed to parse startup confirmation: " << e.what() << std::endl;
        }
        return false;
    }

    void startHeartbeat() {
        heartbeat_alive = true;
        missed_heartbeat_count = 0;
        last_heartbeat_response = std::time(nullptr);

        std::thread([this]() {
            while (heartbeat_alive) {
                // 发送心跳包
                json heartbeat = {
                    {"action", "heartbeat"},
                    {"msg", "ping"}
                };
                
                try {
                    if (send(server_socket, heartbeat.dump().c_str(), heartbeat.dump().size(), 0) < 0) {
                        std::cerr << "Failed to send heartbeat" << std::endl;
                        missed_heartbeat_count++;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error sending heartbeat: " << e.what() << std::endl;
                    missed_heartbeat_count++;
                }

                // 检查是否收到响应
                if (std::time(nullptr) - last_heartbeat_response > 5) {  // 5秒超时
                    missed_heartbeat_count++;
                    std::cout << "No heartbeat response, count: " << missed_heartbeat_count << std::endl;
                }

                // 检查是否需要重连
                if (missed_heartbeat_count >= MAX_MISSED_HEARTBEATS) {
                    std::cout << "Max missed heartbeats reached, attempting reconnection..." << std::endl;
                    close(server_socket);
                    if (reconnectToPort(port)) {
                        // 重连成功，重置计数器
                        missed_heartbeat_count = 0;
                        last_heartbeat_response = std::time(nullptr);
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(3));  // 每3秒发送一次心跳
            }
        }).detach();
    }

    void handleHeartbeatResponse(const json& j) {
        if (j["action"] == "heartbeat" && j["msg"] == "pong") {
            missed_heartbeat_count = 0;  // 重置计数器
            last_heartbeat_response = std::time(nullptr);
            std::cout << "Received heartbeat response" << std::endl;
        }
    }

    void receiveMessages() {
        char buffer[1024];
        while (true) {
            ssize_t numBytes = recv(server_socket, buffer, sizeof(buffer) - 1, 0);
            if (numBytes <= 0) {
                if (numBytes == 0 || (numBytes == -1 && (errno == ECONNRESET))) {
                    // 连接关闭或重置
                    close(server_socket);
                    std::cout << "Connection closed. Attempting to reconnect..." << std::endl;
                    if (reconnectToPort(port)) {
                        missed_heartbeat_count = 0;  // 重连成功后重置计数器
                        last_heartbeat_response = std::time(nullptr);
                    }
                    continue;
                } else {
                    // 其他读取错误
                    close(server_socket);
                    std::cerr << "recv() failed: " << strerror(errno) << std::endl;
                    break;
                }
            } else {
                buffer[numBytes] = '\0';
                std::cout << "Received message: " << buffer << std::endl;
                
                try {
                    auto j = json::parse(buffer);
                    handleHeartbeatResponse(j);  // 处理心跳响应
                } catch (const json::parse_error& e) {
                    std::cerr << "Failed to parse message: " << e.what() << std::endl;
                }
            }
        }
    }

public:
    Server() : buffer(SHM_NAME, SHM_SIZE), server_socket(-1), port(0) {} // 初始化端口号变量

    void start() {
        try {
            // 注册信号处理函数
            registerSignalHandlers();

            // 初始化资源
            std::cout << "Initializing resources..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2)); // 模拟初始化过程

            // 从共享内存中读取信息
            std::string info_str = buffer.read();
            json server_info = json::parse(info_str);

            // 解析 JSON 获取端口号
            port = server_info["msg"]["port"].get<unsigned short>();
            if (port == 0) {
                throw std::runtime_error("Invalid port number in shared memory in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
            }

            std::cout << "Read port number from shared memory: " << port << std::endl;

            // 尝试建立连接
            if (!connectToPort(port)) {
                throw std::runtime_error("Failed to connect to the specified port in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
            }

            // 发送启动消息
            sendMessage("startup", port);
            std::cout << "Startup message sent, waiting for confirmation..." << std::endl;

            // 等待守护进程确认
            if (!waitForStartupConfirmation()) {
                throw std::runtime_error("Failed to receive startup confirmation");
            }

            // 收到确认后再启动心跳
            startHeartbeat();
            std::cout << "Server started successfully" << std::endl;

            // 启动一个线程来接收消息
            std::thread receiveThread(&Server::receiveMessages, this);
            receiveThread.detach();

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
                    close(server_socket);
                    return; // 正常退出
                }

                // 模拟异常情况
                // if (rand() % 10 == 0) {
                //     sendMessageToGuardian("OnStopped Exception");
                //     std::cerr << "Server encountered an exception" << std::endl;
                //     close(server_socket);
                //     exit(1); // 异常退出
                // }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception occurred: " << e.what() << std::endl;
            if (server_socket != -1) {
                close(server_socket);
            }
            exit(EXIT_FAILURE);
        }
    }
};

int main() {
    srand(time(nullptr)); // 初始化随机数种子
    try {
        Server server;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred in main: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
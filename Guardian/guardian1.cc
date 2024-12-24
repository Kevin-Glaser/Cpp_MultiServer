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

// 写日志
static void writeLog(const std::string& message) {
    std::ofstream log("/home/demo/Documents/Cpp_MultiServer/Guardian/guardian.log", std::ios_base::app);
    if (log.is_open()) {
        time_t now = time(nullptr);
        char* dt = ctime(&now);
        log << dt << " " << message << std::endl;
        log.close();
    } else {
        std::cerr << "Unable to open log file: /home/demo/Documents/Cpp_MultiServer/Guardian/guardian.log" << std::endl;
    }
}

const char* SHM_NAME = "/shared_memory_example";
const size_t SHM_SIZE = 4096;

// SharedMemoryManager 类
class SharedMemoryManager {
public:
    SharedMemoryManager(const char* name, size_t size)
        : shm_name(name), shm_size(size), shm_fd(-1), shared_data(nullptr) {}

    ~SharedMemoryManager() {
        cleanupSharedMemory();
    }

    bool createSharedMemory() {
        shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            writeLog("Failed to create shared memory: " + std::string(strerror(errno)));
            return false;
        }

        if (ftruncate(shm_fd, shm_size) == -1) {
            writeLog("Failed to set shared memory size: " + std::string(strerror(errno)));
            close(shm_fd);
            return false;
        }

        shared_data = (char*) mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shared_data == MAP_FAILED) {
            writeLog("Failed to map shared memory: " + std::string(strerror(errno)));
            close(shm_fd);
            return false;
        }

        return true;
    }

    void writePort(unsigned short port) {
        sprintf(shared_data, "%u", port);
        writeLog(std::string("set_listening_port_to_shm:[") + shm_name + "][" + std::to_string(port) + "] success");
    }

private:
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

    const char* shm_name;
    size_t shm_size;
    int shm_fd;
    char* shared_data;
};

// SocketManager 类
class SocketManager {
public:
    SocketManager()
        : _listening_fd(-1), _epoll_fd(-1) {}

    ~SocketManager() {
        if (_listening_fd != -1) {
            close(_listening_fd);
        }
        if (_epoll_fd != -1) {
            close(_epoll_fd);
        }
    }

    void start() {
        try {
            setupListeningSocket();
            setupEpoll();
            handleEvents();
        } catch (const std::exception& e) {
            writeLog("SocketManager error: " + std::string(e.what()));
        }
    }

private:
    void setupListeningSocket() {
        struct sockaddr_in addrServ;
        socklen_t ret;

        _listening_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_listening_fd == -1) {
            throw std::runtime_error("socket() failed");
        }

        // 设置地址复用
        int optval = 1;
        setsockopt(_listening_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        addrServ.sin_family = AF_INET;
        addrServ.sin_port = htons(0); // 让系统选择一个可用的端口
        addrServ.sin_addr.s_addr = INADDR_ANY;

        ret = bind(_listening_fd, (struct sockaddr*)&addrServ, sizeof(struct sockaddr_in));
        if (ret) {
            close(_listening_fd);
            throw std::runtime_error("bind() failed");
        }

        ret = listen(_listening_fd, 10);
        if (ret) {
            close(_listening_fd);
            throw std::runtime_error("listen() failed");
        }

        // 将端口号写入共享内存
        unsigned short localPort = getLocalPort(_listening_fd);
        SharedMemoryManager shm_manager(SHM_NAME, SHM_SIZE);
        if (!shm_manager.createSharedMemory()) {
            throw std::runtime_error("Failed to create shared memory");
        }
        shm_manager.writePort(localPort);

        // 设置为非阻塞模式
        int flags = fcntl(_listening_fd, F_GETFL, 0);
        fcntl(_listening_fd, F_SETFL, flags | O_NONBLOCK);
    }

    unsigned short getLocalPort(int sockfd) {
        struct sockaddr_in sockAddr;
        socklen_t nlen = sizeof(sockAddr);
        getsockname(sockfd, (struct sockaddr*)&sockAddr, &nlen);
        return ntohs(sockAddr.sin_port);
    }

    void setupEpoll() {
        _epoll_fd = epoll_create1(0);
        if (_epoll_fd == -1) {
            throw std::runtime_error("epoll_create1() failed");
        }

        struct epoll_event event;
        event.events = EPOLLIN | EPOLLET; // 边缘触发
        event.data.fd = _listening_fd;

        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _listening_fd, &event) == -1) {
            close(_epoll_fd);
            throw std::runtime_error("epoll_ctl() failed");
        }
    }

    void handleEvents() {
        const int MAX_EVENTS = 10;
        struct epoll_event events[MAX_EVENTS];

        while (true) {
            int nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, -1);
            if (nfds == -1) {
                if (errno != EINTR) { // 如果不是被信号中断，则抛出异常
                    throw std::runtime_error("epoll_wait() failed");
                }
                continue;
            }

            for (int i = 0; i < nfds; ++i) {
                if (events[i].data.fd == _listening_fd) {
                    // 接受新连接
                    struct sockaddr_in clientAddr;
                    socklen_t addrlen = sizeof(clientAddr);
                    int connfd = accept(_listening_fd, (struct sockaddr*)&clientAddr, &addrlen);
                    if (connfd == -1) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) { // 非阻塞错误
                            throw std::runtime_error("accept() failed");
                        }
                        break;
                    }

                    // 设置新连接为非阻塞模式
                    int flags = fcntl(connfd, F_GETFL, 0);
                    fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

                    // 添加到 epoll 监听
                    struct epoll_event event;
                    event.events = EPOLLIN | EPOLLET;
                    event.data.fd = connfd;

                    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, connfd, &event) == -1) {
                        close(connfd);
                        throw std::runtime_error("epoll_ctl() failed");
                    }

                    writeLog("New connection established.");
                } else {
                    // 处理客户端请求
                    int fd = events[i].data.fd;
                    char buffer[1024];
                    ssize_t numBytes = read(fd, buffer, sizeof(buffer) - 1);
                    if (numBytes <= 0) {
                        if (numBytes == 0 || (numBytes == -1 && (errno == ECONNRESET))) {
                            // 连接关闭或重置
                            close(fd);
                            epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                            writeLog("Connection closed.");
                        } else {
                            // 其他读取错误
                            close(fd);
                            epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                            throw std::runtime_error("read() failed");
                        }
                    } else {
                        buffer[numBytes] = '\0';
                        // 处理收到的数据
                        writeLog("Received data from client: " + std::string(buffer));
                        // 回应客户端（这里仅作为示例）
                        write(fd, "Echo: ", 6);
                        write(fd, buffer, numBytes);
                    }
                }
            }
        }
    }

    int _listening_fd;
    int _epoll_fd;
};

int main() {
    try {
        // 启动 SocketManager 并进入事件循环
        SocketManager socket_manager;
        socket_manager.start();
    } catch (const std::exception& e) {
        writeLog("Main error: " + std::string(e.what()));
    }

    return 0;
}
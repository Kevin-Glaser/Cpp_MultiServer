#include "SocketManager.h"


void SocketManager::start() {
    try {
        if(impl) {
            impl->setupListeningSocket();
            impl->setupEpoll();
            impl->startChildProcess();
            impl->handleEvents();
        }
    } catch (const std::exception& e) {
        writeLog("SocketManager error: " + std::string(e.what()));
    }
}

SocketManager::SocketManagerImpl::SocketManagerImpl(const char* shm_name, const size_t shm_size, const char* server_path) : long_listening_fd(-1), _epoll_fd(-1), shm_manager(std::make_unique<SharedMemoryManager>(shm_name, shm_size)), server_path(server_path) {
        // 设置信号处理函数
        signal(SIGINT, SocketManagerImpl::signalHandler);
        signal(SIGTERM, SocketManagerImpl::signalHandler);
        std::cout << "SocketManagerImpl created." << std::endl;
    }

void SocketManager::SocketManagerImpl::setupListeningSocket() {
    initSocketAddr(long_listening_fd);

    // 获取长连接的端口号
    unsigned short longPort = getLocalPort(long_listening_fd);

    // 使用 writePort 函数写入端口号
    if (!shm_manager->createSharedMemory()) {
        throw std::runtime_error("Failed to create shared memory in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    }

    json server_info = {
        {"action", "start_server"},
        {"msg", 
            {
                {"server_id", "server-01"},
                {"version", "1.0.0"},
                {"port", longPort},
            }
        }
    };

    std::string json_str = server_info.dump();
    shm_manager->writeData(json_str); // shortPort 设置为 0 表示无短连接

    // 设置为非阻塞模式
    int flags = fcntl(long_listening_fd, F_GETFL, 0);
    fcntl(long_listening_fd, F_SETFL, flags | O_NONBLOCK);
}

void SocketManager::SocketManagerImpl::initSocketAddr(int& sockfd) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        throw std::runtime_error("socket() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    }

    // 设置地址复用
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0); // 让系统选择一个可用的端口
    addr.sin_addr.s_addr = INADDR_ANY;

    socklen_t ret = bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
    if (ret) {
        close(sockfd);
        throw std::runtime_error("bind() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    }

    ret = listen(sockfd, 10);
    if (ret) {
        close(sockfd);
        throw std::runtime_error("listen() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    }
}

void SocketManager::SocketManagerImpl::stopChildProcess()  {
    if (child_pid > 0) {
        kill(child_pid, SIGTERM);
        waitpid(child_pid, nullptr, 0);
        writeLog("Child process stopped.");
    }
}

unsigned short SocketManager::SocketManagerImpl::getLocalPort(int sockfd) {
    struct sockaddr_in sockAddr;
    socklen_t nlen = sizeof(sockAddr);
    getsockname(sockfd, (struct sockaddr*)&sockAddr, &nlen);
    return ntohs(sockAddr.sin_port);
}

void SocketManager::SocketManagerImpl::setupEpoll() {
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1) {
        throw std::runtime_error("epoll_create1() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // 边缘触发
    event.data.fd = long_listening_fd;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, long_listening_fd, &event) == -1) {
        close(_epoll_fd);
        throw std::runtime_error("epoll_ctl() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    }
}

void SocketManager::SocketManagerImpl::sendMessage(const std::string& message) {
    ssize_t sent_bytes = send(server_fds, message.c_str(), message.size(), 0);
    if (sent_bytes == -1) {
        writeLog("Failed to send message to client with fd: " + std::to_string(server_fds));
    } else {
        writeLog("Message sent to client with fd: " + std::to_string(server_fds));
    }
}

void SocketManager::SocketManagerImpl::handleEvents() {
    const int MAX_EVENTS = 1;
    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno != EINTR) { // 如果不是被信号中断，则抛出异常
                throw std::runtime_error("epoll_wait() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
            }
            continue;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == long_listening_fd) {
                // 接受新连接
                struct sockaddr_in clientAddr;
                socklen_t addrlen = sizeof(clientAddr);
                int connfd = accept(long_listening_fd, (struct sockaddr*)&clientAddr, &addrlen);
                if (connfd == -1) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) { // 非阻塞错误
                        throw std::runtime_error("accept() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
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
                    throw std::runtime_error("epoll_ctl() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
                }
                server_fds = connfd;
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
                        writeLog("Connection closed. Attempting to reconnect...");
                        handleReconnection();
                    } else {
                        // 其他读取错误
                        close(fd);
                        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                        throw std::runtime_error("read() failed");
                    }
                } else {
                    buffer[numBytes] = '\0';
                    writeLog("Received data from server: " + std::string(buffer));
                    // 解析 JSON 消息
                    try {
                        json j = json::parse(buffer);
                        handleAction(j["action"], j["msg"]);
                    } catch (const json::parse_error& e) {
                        writeLog("Failed to parse JSON message: " + std::string(e.what()));
                    }
                }
            }
        }
        // 添加日志记录以确保持续读取消息
        sleep(1);
    }
}

std::string SocketManager::SocketManagerImpl::actionToString(Action action) {
    switch (action) {
        case Action::START:
            return "startup";
        case Action::STOP:
            return "shutdown";
        case Action::HEARTBEAT:
            return "heartbeat";
        default:
            return "unknown";
    }
}

// 根据 action 调用相应函数
void SocketManager::SocketManagerImpl::handleAction(const std::string& action, const std::string& msg) {
    if (action == "startup") {
        handleStart(msg);
    } else if (action == "shutdown") {
        handleStop(msg);
    } else {
        writeLog("Unknown action: " + action);
    }
}

void SocketManager::SocketManagerImpl::handleStart(const std::string& msg) {
    writeLog("Start action received: " + msg);
    if(std::string(msg).find("connected") != std::string::npos) {
        writeLog("Shared memory will be closed to save memory");
        shm_manager->cleanupSharedMemory();
        startHeartbeat();
    }
}

void SocketManager::SocketManagerImpl::handleStop(const std::string& msg) {
    writeLog("Stop action received: " + msg);
    stopChildProcess();
}

void SocketManager::SocketManagerImpl::startChildProcess() {
    pid_t pid = fork();
    if (pid == -1) {
        writeLog("Failed to fork process: " + std::string(strerror(errno)));
        throw std::runtime_error("fork() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    } else if (pid == 0) {
        // 子进程
        execl(server_path, "server", (char*)NULL);
        writeLog("Failed to execute server: " + std::string(strerror(errno)));
        exit(1);
    } else {
        child_pid = pid;
        // 父进程
        writeLog("Parent process continues.");
    }
}

void SocketManager::SocketManagerImpl::handleReconnection() {
    while (true) {
        // 创建 JSON 消息
        json reconnectMsg = {
            {"action", "reconnect"},
            {"msg", "reconnection attempt"}
        };
        std::string reconnectMsgStr = reconnectMsg.dump();
        sendMessage(reconnectMsgStr);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void SocketManager::SocketManagerImpl::startHeartbeat() {
    while (true) {
        json heartbeatMsg = {
            {"action", "ping"},
            {"msg", "heartbeat message"}
        };
        std::string heartbeatMsgStr = heartbeatMsg.dump();
        sendMessage(heartbeatMsgStr);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
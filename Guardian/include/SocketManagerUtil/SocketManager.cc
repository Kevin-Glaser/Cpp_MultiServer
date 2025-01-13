#include "SocketManager.h"

bool SharedMemoryManager::createSharedMemory()  {
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

void SharedMemoryManager::writePort(unsigned short port) {
    json server_info = {
        {"action", "start_server"},
        {"msg", 
            {
                {"server_id", "server-01"},
                {"version", "1.0.0"},
                {"port", port},
            }
        }
    };

    std::string json_str = server_info.dump();
    sprintf(shared_data, "%s", json_str.c_str());
    writeLog(std::string("set_listening_port_to_shm:[") + shm_name + "][" + json_str + "] success");
}

void SharedMemoryManager::cleanupSharedMemory() {
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
    if (shared_data != nullptr) {
        munmap(shared_data, shm_size);
        shared_data = nullptr;
    }
}



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

SocketManager::SocketManagerImpl::SocketManagerImpl(const char* shm_name, const size_t shm_size, const char* server_path) : _listening_fd(-1), _epoll_fd(-1), shm_manager(std::make_unique<SharedMemoryManager>(shm_name, shm_size)), server_path(server_path) {
        // 设置信号处理函数
        signal(SIGINT, SocketManagerImpl::signalHandler);
        signal(SIGTERM, SocketManagerImpl::signalHandler);
        std::cout << "SocketManagerImpl created." << std::endl;
    }

void SocketManager::SocketManagerImpl::setupListeningSocket () {
    struct sockaddr_in addrServ;
    socklen_t ret;

    _listening_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listening_fd == -1) {
        throw std::runtime_error("socket() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
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
        throw std::runtime_error("bind() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    }

    ret = listen(_listening_fd, 10);
    if (ret) {
        close(_listening_fd);
        throw std::runtime_error("listen() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    }

    // 将端口号写入共享内存
    unsigned short localPort = getLocalPort(_listening_fd);
    if (!shm_manager->createSharedMemory()) {
        throw std::runtime_error("Failed to create shared memory in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    }
    shm_manager->writePort(localPort);

    // 设置为非阻塞模式
    int flags = fcntl(_listening_fd, F_GETFL, 0);
    fcntl(_listening_fd, F_SETFL, flags | O_NONBLOCK);
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
    event.data.fd = _listening_fd;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _listening_fd, &event) == -1) {
        close(_epoll_fd);
        throw std::runtime_error("epoll_ctl() failed in file " + std::string(__FILE__) + " at line " + std::to_string(__LINE__));
    }
}

void SocketManager::SocketManagerImpl::sendMessage(Action action, const std::string& msg = "") {
    // json j = {
    //     {"action", actionToString(action)},
    //     {"msg", msg}
    // };
    // std::string message = j.dump();
    // sprintf(shared_data, "%s", message.c_str());
    // writeLog(std::string("set_listening_port_to_shm:[") + shm_name + "][" + message + "] success");
}

void SocketManager::SocketManagerImpl::handleEvents() {
    const int MAX_EVENTS = 10;
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
            if (events[i].data.fd == _listening_fd) {
                // 接受新连接
                struct sockaddr_in clientAddr;
                socklen_t addrlen = sizeof(clientAddr);
                int connfd = accept(_listening_fd, (struct sockaddr*)&clientAddr, &addrlen);
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

                client_fds.insert(connfd); // 将新连接的客户端套接字添加到容器中

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
                        client_fds.erase(fd); // 从容器中移除已关闭的客户端套接字
                        writeLog("Connection closed.");
                    } else {
                        // 其他读取错误
                        close(fd);
                        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                        client_fds.erase(fd); // 从容器中移除已关闭的客户端套接字
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

// 将 Action 枚举转换为字符串
std::string SocketManager::SocketManagerImpl::actionToString(Action action) {
    switch (action) {
        case Action::START:
            return "start";
        case Action::STOP:
            return "stop";
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

// 处理启动动作
void SocketManager::SocketManagerImpl::handleStart(const std::string& msg) {
    writeLog("Start action received: " + msg);
    if(std::string(msg).find("connected") != std::string::npos) {
        writeLog("Shared memory will be closed to save memory");
        shm_manager->cleanupSharedMemory();
        startHeartbeat();
    }
}

// 处理停止动作
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
        // 调用server程序
        execl(server_path, "server", (char*)NULL);
        // 如果execl失败，记录错误并退出
        writeLog("Failed to execute server: " + std::string(strerror(errno)));
        exit(1);
    } else {
        child_pid = pid;
        // 父进程
        writeLog("Parent process continues.");
    }
}

void SocketManager::SocketManagerImpl::startHeartbeat() {
    while (true) {
        // 创建 JSON 消息
        json heartbeatMsg = {
            {"action", "ping"},
            {"msg", "heartbeat message"}
        };
        std::string heartbeatMsgStr = heartbeatMsg.dump();

        // 遍历所有连接的客户端套接字并发送心跳消息
        for (int fd : client_fds) {
            ssize_t sent_bytes = send(fd, heartbeatMsgStr.c_str(), heartbeatMsgStr.size(), 0);
            if (sent_bytes == -1) {
                writeLog("Failed to send heartbeat to client with fd: " + std::to_string(fd));
            } else {
                writeLog("Heartbeat sent to client with fd: " + std::to_string(fd));
            }
        }

        // 每隔一秒发送一次心跳消息
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
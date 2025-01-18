# ifndef __SOCKETMANAGER_H__
#define __SOCKETMANAGER_H__

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

#include "SharedMemoryUtil/SharedMemoryUtil.h"
#include "SingletonBase/Singleton.h"
#include "JsonUtil/json.hpp"

using json = nlohmann::json;


// 定义枚举类型
enum class Action {
    START,
    STOP,
    HEARTBEAT
};


class SocketManager {
public:
    void start();

private:
    SocketManager(const char* shm_name, const size_t shm_size, const char* server_path) : impl(std::make_unique<SocketManagerImpl>(shm_name, shm_size, server_path)) {
        std::cout << "SocketManager created." << std::endl;
    }

    ~SocketManager() {
        
    }
private:
    class SocketManagerImpl;
    std::unique_ptr<SocketManagerImpl> impl;
    friend class Singleton<SocketManager>;
};

class SocketManager::SocketManagerImpl {
public:
    SocketManagerImpl(const char* shm_name, const size_t shm_size, const char* server_path);

    ~SocketManagerImpl() {
        if (long_listening_fd != -1) {
            close(long_listening_fd);
        }
        if (_epoll_fd != -1) {
            close(_epoll_fd);
        }
        stopChildProcess();
    }

    void stopChildProcess();
    static void signalHandler(int signum) { exit(signum); }

    void setupListeningSocket();
    void initSocketAddr(int& fd);
    unsigned short getLocalPort(int sockfd);
    void setupEpoll();
    void handleEvents();
    void startChildProcess();
    void handleAction(const std::string& action, const std::string& msg);
    void handleStart(const std::string& msg);
    void handleStop(const std::string& msg);
    void handleReconnection();
    std::string actionToString(Action action);
    void sendMessage(const std::string& msg);
    void startHeartbeat();

    int long_listening_fd;
    int _epoll_fd;
    std::unique_ptr<SharedMemoryManager> shm_manager;
    pid_t child_pid;
    const char* server_path;
    int server_fds; // 保持为单个文件描述符
};

#endif
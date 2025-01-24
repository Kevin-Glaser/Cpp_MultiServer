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
#include "ProcessMonitorUtil/ServerMonitor.h"
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
    std::string actionToString(Action action);
    void sendMessage(const std::string& msg);
    void startHeartbeat();

private:
    int long_listening_fd;
    int _epoll_fd;
    int server_fds;
    std::time_t last_heartbeat_time;
    bool connection_alive;
    const char* server_path;
    pid_t child_pid;
    int heartbeat_failures;
    std::unique_ptr<SharedMemoryManager> shm_manager;
    static const int MAX_HEARTBEAT_FAILURES = 3;
    std::unique_ptr<ServerMonitor> server_monitor;
    
    void handleHeartbeatResponse(const std::string& msg);
    bool isConnectionTimedOut() const;
    void resetHeartbeatTimer();
    void startMonitoring();  // 添加新的监控启动函数
    bool isServerRunning() const;  // 添加服务器运行状态检查
};

#endif
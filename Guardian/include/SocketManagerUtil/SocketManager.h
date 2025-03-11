#ifndef __SOCKETMANAGER_H__
#define __SOCKETMANAGER_H__

#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/epoll.h>
    #include <sys/wait.h>
    #include <sys/types.h>
    #include <signal.h>
#endif

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

#ifdef _WIN32
    using SOCKET_TYPE = SOCKET;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
    using SOCKET_TYPE = int;
    #define INVALID_SOCKET_VALUE (-1)
#endif

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
        if (long_listening_fd != INVALID_SOCKET_VALUE) {
            close(long_listening_fd);
        }
        #ifdef __linux__
        if (_epoll_fd != -1) {
            close(_epoll_fd);
        }
        #endif
        stopChildProcess();
    }

    /**
     * @brief stop server process
     * 
     */
    void stopChildProcess();

    /**
     * @brief handle signal from terminal or others,such as [kill -9] 
     * 
     * @param signum 
     */
    static void signalHandler(int signum) { exit(signum); }

    /**
     * @brief initialize listening socket and write socket port into shared memory
     * 
     */
    void setupListeningSocket();
    
    /**
     * @brief initialize socket address
     * 
     * @param sockfd file descriptor from socket()
     */
    void initSocketAddr(int& fd);

    /**
     * @brief Get the random socket port from system
     * 
     * @param sockfd file descriptor from socket()
     * @return [unsigned short] socket port
     */
    unsigned short getLocalPort(int sockfd);

    /**
     * @brief initialize epoll 
     * 
     */
    void setupEpoll();

    /**
     * @brief handle epoll events
     * 
     */
    void handleEvents();

    /**
     * @brief start child process
     * 
     */
    void startChildProcess();

    /**
     * @brief handle epoll message from server process
     * 
     * @param action json action
     * @param msg json message
     */
    void handleAction(const std::string& action, const std::string& msg);

    /**
     * @brief handle start action, when receive start action from server process, start heartbeat
     * 
     * @param msg 
     */
    void handleStart(const std::string& msg);

    /**
     * @brief handle stop action from server process
     * 
     * @param msg 
     */
    void handleStop(const std::string& msg);

    /**
     * @brief convert enum action to string
     * 
     * @param action 
     * @return [std::string] 
     */
    std::string actionToString(Action action);

    /**
     * @brief send message by tcp socket
     * 
     * @param msg message to be sent
     */
    void sendMessage(const std::string& msg);

    /**
     * @brief initialize connection monitor, watch the heartbeat status
     * 
     */
    void initializeConnectionMonitor();

private:
    SOCKET_TYPE long_listening_fd;
    #ifdef __linux__
    int _epoll_fd;
    #endif
    SOCKET_TYPE server_fds;
    std::time_t last_heartbeat_time;
    bool connection_alive;
    const char* server_path;
    pid_t child_pid;
    std::unique_ptr<SharedMemoryManager> shm_manager;
    std::unique_ptr<ServerMonitor> server_monitor;
    
    void handleHeartbeatRequest();
    bool isConnectionTimedOut() const;
    void resetHeartbeatTimer();
    void startProcessMonitoring();
    bool isServerRunning() const;
};

#endif
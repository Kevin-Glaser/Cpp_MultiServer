#ifndef __SERVER_UTIL_H__
#define __SERVER_UTIL_H__

#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <signal.h>

#include "../JsonUtil/json.hpp"

using json = nlohmann::json;

class SharedMemoryBuffer {
private:
    int shm_fd;
    const char* name;    // 移到前面
    const size_t size;   // 移到前面
    char* data;         // 移到最后

public:
    SharedMemoryBuffer(const char* name, size_t size);
    ~SharedMemoryBuffer();
    void write(const std::string& message);
    std::string read() const;
    char* getData() const;
};

class ServerUtil {
private:
    SharedMemoryBuffer buffer;
    int server_socket;
    unsigned short port;

    static const int MAX_MISSED_HEARTBEATS = 3;
    int missed_heartbeat_count;
    bool heartbeat_alive;
    std::time_t last_heartbeat_response;

    void sendMessageToGuardian(const std::string& message);
    static void handleSignal(int signum, siginfo_t* info, void* context);
    void registerSignalHandlers();
    bool connectToPort(unsigned short port);
    bool reconnectToPort(unsigned short port);
    void sendMessage(const char* action, unsigned short port);
    bool waitForStartupConfirmation();
    void startHeartbeat();
    void handleHeartbeatResponse(const json& j);
    void receiveMessages();

public:
    ServerUtil();
    void start();
};

#endif
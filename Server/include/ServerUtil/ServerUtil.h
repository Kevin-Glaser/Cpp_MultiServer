#ifndef __SERVER_UTIL_H__
#define __SERVER_UTIL_H__

#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>

#ifdef WIN32_PLATFORM
    #include <winsock2.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <signal.h>
#endif

#include "../JsonUtil/json.hpp"

using json = nlohmann::json;

class SharedMemoryBuffer {
private:
    #ifdef WIN32_PLATFORM
        HANDLE hMapFile;
        LPVOID pBuf;
    #else
        int shm_fd;
        char* data;
    #endif
    const char* name;
    const size_t size;

public:
    SharedMemoryBuffer(const char* name, size_t size);
    ~SharedMemoryBuffer();
    void write(const std::string& message);
    std::string read() const;
    #ifdef WIN32_PLATFORM
        LPVOID getData() const { return pBuf; }
    #else
        char* getData() const { return data; }
    #endif
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

    /**
     * @brief send message to guardian by shared memory
     * 
     * @param message message to be sent
     */
    void sendMessageToGuardian(const std::string& message);

    /**
     * @brief handle signal from terminal
     * 
     * @param signum 
     * @param info 
     * @param context 
     */
    static void handleSignal(int signum, siginfo_t* info, void* context);
    void registerSignalHandlers();
    bool connectToPort(unsigned short port);
    bool reconnectToPort(unsigned short port);
    void sendMessage(const char* action, unsigned short port);
    bool waitForStartupConfirmation();

    /**
     * @brief startup heartbeat and send heartbeat message to guardian by socket
     *        if the count of missed heartbeats reaches the maximum, reconnect to the port
     * 
     */
    void startHeartbeat();

    /**
     * @brief receive and parse the json message from guardian, if success, reset the count of missed heartbeats
     * 
     * @param j message from guardian in json format
     */
    void handleHeartbeatResponse(const json& j);
    void receiveMessages();

public:
    ServerUtil();

    /**
     * @brief startup the tcp connection and heartbeat with guardian
     * 
     */
    void start();
};

#endif
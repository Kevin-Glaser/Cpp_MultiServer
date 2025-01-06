#include "SocketManagerUtil/SocketManager.h"

int main() {
    try {
        const char* shm_name = "test_shm";
        const size_t shm_size = 4096;
        const char* server_path = "/home/demo/Documents/Cpp_MultiServer/Guardian/server";
        // 启动 SocketManager 并进入事件循环
        SocketManager& manager = Singleton<SocketManager>::GetInstance(shm_name, shm_size, server_path);
        manager.start();
    } catch (const std::exception& e) {
        writeLog("Main error: " + std::string(e.what()));
    }

    return 0;
}
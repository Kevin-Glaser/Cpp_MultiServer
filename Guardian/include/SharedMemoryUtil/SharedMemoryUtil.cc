#include "SharedMemoryUtil/SharedMemoryUtil.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

bool SharedMemoryManager::createSharedMemory() {
#ifdef _WIN32
    // Windows shared memory implementation
    std::string mapName = std::string(shm_name);
    hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        shm_size,
        mapName.c_str());

    if (hMapFile == NULL) {
        writeLog("Failed to create file mapping: " + std::to_string(GetLastError()));
        return false;
    }

    shared_data = (char*)MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        shm_size);

    if (shared_data == NULL) {
        writeLog("Failed to map view of file: " + std::to_string(GetLastError()));
        CloseHandle(hMapFile);
        return false;
    }
#else
    // Unix/Linux shared memory implementation
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

    shared_data = (char*)mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        writeLog("Failed to map shared memory: " + std::string(strerror(errno)));
        close(shm_fd);
        return false;
    }
#endif
    return true;
}

void SharedMemoryManager::writeData(const std::string& jsonStr) {
#ifdef _WIN32
    memcpy_s(shared_data, shm_size, jsonStr.c_str(), jsonStr.length() + 1);  // +1 for null terminator
#else
    snprintf(shared_data, shm_size, "%s", jsonStr.c_str());
#endif
    writeLog(std::string("set_listening_port_to_shm:[") + shm_name + "][" + jsonStr + "] success");
}

void SharedMemoryManager::cleanupSharedMemory() {
#ifdef _WIN32
    if (shared_data != nullptr) {
        UnmapViewOfFile(shared_data);
        shared_data = nullptr;
    }
    if (hMapFile != NULL) {
        CloseHandle(hMapFile);
        hMapFile = NULL;
    }
#else
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
    if (shared_data != nullptr) {
        munmap(shared_data, shm_size);
        shared_data = nullptr;
    }
#endif
}
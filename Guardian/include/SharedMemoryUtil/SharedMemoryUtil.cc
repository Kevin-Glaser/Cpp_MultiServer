#include "SharedMemoryUtil/SharedMemoryUtil.h"


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

void SharedMemoryManager::writeData(const std::string& jsonStr) {
    sprintf(shared_data, "%s", jsonStr.c_str());
    writeLog(std::string("set_listening_port_to_shm:[") + shm_name + "][" + jsonStr + "] success");
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
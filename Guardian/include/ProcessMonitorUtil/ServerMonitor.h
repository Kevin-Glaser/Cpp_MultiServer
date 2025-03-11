/**
 * @file ServerMonitor.h
 * @author KevinGlaser
 * @brief This is a monitor to check the system resources use of a specific server process
 * @version 0.1
 * @date 2025-03-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __SERVER_MONITOR_H__
#define __SERVER_MONITOR_H__

#include <chrono>
#include <string>
#include <fstream>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #include <pdh.h>
    #pragma comment(lib, "pdh.lib")
#else
    #include <sys/resource.h>
    #include <sys/sysinfo.h>
    #include <sys/times.h>
    #include <dirent.h>
#endif

#include "JsonUtil/json.hpp"

using json = nlohmann::json;

class ServerMonitor {
public:
    struct ServerStatus {
        double cpu_usage;          // CPU使用率 (%)
        double memory_usage;       // 内存使用率 (%)
        double load_average;       // 系统负载
        bool is_deadlocked;       // 是否检测到死锁
        time_t last_active_time;   // 最后活动时间
        int thread_count;          // 线程数量
        
        ServerStatus() : cpu_usage(0), memory_usage(0), load_average(0),
                        is_deadlocked(false), last_active_time(0), thread_count(0) {}
    };

    #ifdef _WIN32
        explicit ServerMonitor(DWORD monitored_pid);
    #else
        explicit ServerMonitor(pid_t monitored_pid);
    #endif

    ~ServerMonitor() {
        #ifdef _WIN32
            if (cpu_query) PdhCloseQuery(cpu_query);
        #endif
    }

    // 主要监控函数
    ServerStatus checkServerStatus();
    bool isHealthy() const;
    std::string getStatusReport() const;

private:
    #ifdef _WIN32
        DWORD pid;
        PDH_HQUERY cpu_query;
        PDH_HCOUNTER cpu_counter;
    #else
        pid_t pid;
    #endif

    ServerStatus current_status;    // 当前状态
    std::chrono::steady_clock::time_point last_cpu_check;
    unsigned long last_cpu_total;   // 上次CPU统计总量
    unsigned long last_cpu_proc;    // 上次进程CPU使用量

    static const int CPU_THRESHOLD = 90;      // CPU使用率警告阈值 (%)
    static const int MEMORY_THRESHOLD = 85;   // 内存使用率警告阈值 (%)
    static const int DEADLOCK_TIMEOUT = 60;   // 死锁检测超时时间 (秒)
    
    /**
     * @brief Get the Cpu Usage object
     * 
     * @return double return the CPU usage of the server process
     */
    double getCpuUsage();

    /**
     * @brief Get the Memory Usage object
     * 
     * @return double return the memory usage of the server process
     */
    double getMemoryUsage();

    /**
     * @brief Get the System Load Average object
     * 
     * @return double return the system load average
     */
    double getSystemLoadAverage();
    
    /**
     * @brief check if server process has been locked
     * 
     * @return true return locked
     * @return false return unlocked
     */
    bool checkDeadlock();

    /**
     * @brief Get the Thread Count object of server process
     * 
     * @return int return the thread count of server process
     */
    int getThreadCount();

    /**
     * @brief update the last active time of server process
     * 
     */
    void updateLastActiveTime();
    
    /**
     * @brief tool function to read the process CPU time of last time
     * 
     * @return [unsigned long] return the process CPU time of last time
     */
    unsigned long readProcessCpuTime();

    /**
     * @brief tool function to read the  total CPU time of last time
     * 
     * @return [unsigned long] return the total CPU time of last time
     */
    unsigned long readTotalCpuTime();
};

#endif


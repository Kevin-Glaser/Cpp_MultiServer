#ifndef __SERVER_MONITOR_H__
#define __SERVER_MONITOR_H__

#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/times.h>
#include <fstream>
#include <chrono>
#include <dirent.h>

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

    explicit ServerMonitor(pid_t monitored_pid);
    ~ServerMonitor() = default;

    // 主要监控函数
    ServerStatus checkServerStatus();
    bool isHealthy() const;
    std::string getStatusReport() const;

private:
    pid_t pid;                     // 被监控的进程ID
    ServerStatus current_status;    // 当前状态
    std::chrono::steady_clock::time_point last_cpu_check;
    unsigned long last_cpu_total;   // 上次CPU统计总量
    unsigned long last_cpu_proc;    // 上次进程CPU使用量

    static const int CPU_THRESHOLD = 90;      // CPU使用率警告阈值 (%)
    static const int MEMORY_THRESHOLD = 85;   // 内存使用率警告阈值 (%)
    static const int DEADLOCK_TIMEOUT = 60;   // 死锁检测超时时间 (秒)
    
    // 具体监控函数
    double getCpuUsage();
    double getMemoryUsage();
    double getSystemLoadAverage();
    bool checkDeadlock();
    int getThreadCount();
    void updateLastActiveTime();
    
    // 辅助函数
    unsigned long readProcessCpuTime();
    unsigned long readTotalCpuTime();
};

#endif


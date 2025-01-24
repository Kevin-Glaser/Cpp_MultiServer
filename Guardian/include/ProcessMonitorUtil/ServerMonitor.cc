#include "ServerMonitor.h"

// ServerMonitor 实现
ServerMonitor::ServerMonitor(pid_t monitored_pid) : pid(monitored_pid) {
    last_cpu_check = std::chrono::steady_clock::now();
    last_cpu_total = readTotalCpuTime();
    last_cpu_proc = readProcessCpuTime();
}

double ServerMonitor::getCpuUsage() {
    auto current_time = std::chrono::steady_clock::now();
    unsigned long current_cpu_total = readTotalCpuTime();
    unsigned long current_cpu_proc = readProcessCpuTime();
    
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>
                    (current_time - last_cpu_check).count();
    
    if (time_diff == 0) return 0.0;
    
    double cpu_usage = 100.0 * (current_cpu_proc - last_cpu_proc) / 
                      (current_cpu_total - last_cpu_total);
    
    last_cpu_check = current_time;
    last_cpu_total = current_cpu_total;
    last_cpu_proc = current_cpu_proc;
    
    return std::max(0.0, std::min(100.0, cpu_usage));
}

double ServerMonitor::getMemoryUsage() {
    std::ifstream statm("/proc/" + std::to_string(pid) + "/statm");
    if (!statm.is_open()) return 0.0;
    
    unsigned long vm_size, resident;
    statm >> vm_size >> resident;
    statm.close();
    
    struct sysinfo si;
    if (sysinfo(&si) != 0) return 0.0;
    
    return (resident * 100.0) / (si.totalram / si.mem_unit);
}

bool ServerMonitor::checkDeadlock() {
    std::string proc_status = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream status(proc_status);
    if (!status.is_open()) return true; // 如果无法读取进程状态，认为进程可能已经死锁
    
    std::string line;
    std::string state;
    while (std::getline(status, line)) {
        if (line.substr(0, 6) == "State:") {
            state = line.substr(7);
            break;
        }
    }
    
    // 检查进程状态，D状态表示不可中断的睡眠状态，可能表示死锁
    if (state.find('D') != std::string::npos) {
        time_t current_time = std::time(nullptr);
        if (current_time - current_status.last_active_time > DEADLOCK_TIMEOUT) {
            return true;
        }
    }
    
    return false;
}

unsigned long ServerMonitor::readProcessCpuTime() {
    std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
    if (!stat.is_open()) return 0;
    
    std::string unused;
    unsigned long utime, stime;
    for (int i = 1; i < 14; ++i) stat >> unused;
    stat >> utime >> stime;
    
    return utime + stime;
}

unsigned long ServerMonitor::readTotalCpuTime() {
    std::ifstream stat("/proc/stat");
    if (!stat.is_open()) return 0;
    
    std::string cpu;
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
    stat >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

ServerMonitor::ServerStatus ServerMonitor::checkServerStatus() {
    current_status.cpu_usage = getCpuUsage();
    current_status.memory_usage = getMemoryUsage();
    current_status.is_deadlocked = checkDeadlock();
    current_status.thread_count = getThreadCount();
    updateLastActiveTime();
    
    return current_status;
}

bool ServerMonitor::isHealthy() const {
    return !current_status.is_deadlocked && 
           current_status.cpu_usage < CPU_THRESHOLD && 
           current_status.memory_usage < MEMORY_THRESHOLD;
}

std::string ServerMonitor::getStatusReport() const {
    json report = {
        {"cpu_usage", current_status.cpu_usage},
        {"memory_usage", current_status.memory_usage},
        {"thread_count", current_status.thread_count},
        {"is_deadlocked", current_status.is_deadlocked},
        {"health_status", isHealthy() ? "healthy" : "unhealthy"}
    };
    
    return report.dump();
}

int ServerMonitor::getThreadCount() {
    std::string task_dir = "/proc/" + std::to_string(pid) + "/task";
    int count = 0;
    
    DIR* dir = opendir(task_dir.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_name[0] != '.') {
                count++;
            }
        }
        closedir(dir);
    }
    
    return count;
}

void ServerMonitor::updateLastActiveTime() {
    current_status.last_active_time = std::time(nullptr);
}
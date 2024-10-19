#include <algorithm>
#include <sstream>
#include <ctime>
#include <thread>
#include <iomanip>

#include "LogUtil.h"

void Logger::AddHandler(std::unique_ptr<LogHandler> handler) {
    p_log->p_handlers.emplace_back(std::move(handler));
}

void Logger::RemoveHandler(std::unique_ptr<LogHandler>&& handler) {
    auto it = std::find(p_log->p_handlers.begin(), p_log->p_handlers.end(), handler);
    if(it != p_log->p_handlers.end()) {
        it = p_log->p_handlers.erase(it);
    }
} 

void Logger::WriteLog(const std::string& message, LogLevel level) {
    std::time_t now = std::time(nullptr);
    std::tm* now_tm = std::localtime(&now);

    std::stringstream ss;
    ss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");

    std::string time_str = ss.str();
    std::string full_message = time_str + " [" + LogLevelToString(level) + "] " + "log_queue_size:" + std::to_string(p_log->log_queues.size()) + " message:" + message;

    std::lock_guard<std::mutex> lck(p_log->qeueue_mtx);
    p_log->log_queues.emplace(full_message, level);
    p_log->queue_cv.notify_one();
}

Logger::~Logger()
{
    std::cout << "~Logger" << std::endl << std::flush;
}

Logger::LoggerImpl::~LoggerImpl()
{
    std::cout << "~LoggerImpl" << std::endl << std::flush;
    
    {
        std::lock_guard<std::mutex> lock(qeueue_mtx);
        log_queues.emplace("", LogLevel::ERROR);  // 放一个空消息作为停止信号
        should_stop = true;
    }
    queue_cv.notify_one();

    // 等待线程完成
    if (work_thread_ptr && work_thread_ptr->joinable()) {
        work_thread_ptr->join();
    }
}

void Logger::LoggerImpl::WorkThread() {
    while(true) {
        std::string message;
        LogLevel level;
        {
            std::unique_lock<std::mutex> lck(qeueue_mtx);
            queue_cv.wait(lck, [this]{return !log_queues.empty() || should_stop; });
            if(should_stop == true) {
                break;
            }
            std::tie(message, level) = log_queues.front();
            log_queues.pop();
        }

        for(auto& handler : p_handlers) {
            handler->HandleLog(message, level);
        }
    }
}

void TerminalLogHandler::HandleLog(const std::string& message, LogLevel level) {
    std::cout << std::this_thread::get_id() << " " << message << std::endl << std::flush;
}

void FileLogHandler::HandleLog(const std::string& message, LogLevel level) {
    if (!file_.is_open()) {
        std::cerr << "File not open" << std::endl;
        return;
    }
    file_ << std::this_thread::get_id() << " " << message << std::endl << std::flush;
}

std::string LogLevelToString(LogLevel level) {
    switch (level) {
        case WARN: return "WARN";
        case ERROR: return "ERROR";
        case INFO: return "INFO";
        case DEBUG: return "DEBUG";
    }
    return "UNKNOWN";
}
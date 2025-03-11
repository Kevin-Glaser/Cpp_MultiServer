/**
 * @file LogUtil.h
 * @author KevinGlaser
 * @brief LogUtil class for logging messages to terminal or file
 * @version 0.1
 * @date 2025-03-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __LOGUTIL_H__
#define __LOGUTIL_H__

#include <memory>
#include <iostream>
#include <vector>
#include <fstream>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "SingletonBase/Singleton.h"

enum LogLevel{
    WARN,
    ERROR,
    INFO,
    DEBUG
};

enum LogTarget{
    TERMINAL,
    LOGFILE,
    TERMINAL_FILE
};

class LogHandler {
public:
    LogHandler() { std::cout << "LogHandler" << std::endl << std::flush; }
    virtual void HandleLog(const std::string& message, LogLevel level) = 0;
    virtual ~LogHandler() {
        std::cout << "~LogHandler" << std::endl << std::flush;
    }
};

class TerminalLogHandler : public LogHandler {
public:
    TerminalLogHandler() { std::cout << "TerminalLogHandler" << std::endl << std::flush; }
    void HandleLog(const std::string& message, LogLevel level) override;
    virtual ~TerminalLogHandler() override { std::cout << "~TerminalLogHandler" << std::endl << std::flush; }
};

class FileLogHandler : public LogHandler {
public:
    explicit FileLogHandler(const std::string& filename) : file_(filename, std::ios::out | std::ios::app)
     {std::cout << "FileLogHandler" << std::endl << std::flush;}
    virtual ~FileLogHandler() override { 
        if(file_.is_open()) {
            file_.close();
        }
        std::cout << "~FileLogHandler" << std::endl << std::flush; }

    void HandleLog(const std::string& message, LogLevel level) override;
private:
    std::ofstream file_;
};

class Logger final : public Singleton<Logger>{
public:
    void AddHandler(std::unique_ptr<LogHandler> handler);
    void RemoveHandler(std::unique_ptr<LogHandler>&& handler);
    void WriteLog(const std::string& message, LogLevel level);
private:
    Logger() : p_log(std::make_unique<LoggerImpl>()) { std::cout << "Logger" << std::endl << std::flush; }
    virtual ~Logger();

    friend class Singleton<Logger>;
    class LoggerImpl;
    std::unique_ptr<LoggerImpl> p_log;
    
};

std::string LogLevelToString(LogLevel level);

class Logger::LoggerImpl {
public:
    LoggerImpl() : work_thread_ptr(std::make_shared<std::thread>(&LoggerImpl::WorkThread, this)), should_stop(false) { std::cout << "LoggerImpl" << std::endl << std::flush; }
    virtual ~LoggerImpl();

    void WorkThread();

    std::vector<std::unique_ptr<LogHandler>> p_handlers;
    std::queue<std::pair<std::string, LogLevel>> log_queues;
    std::mutex qeueue_mtx;
    std::condition_variable queue_cv;
    std::shared_ptr<std::thread> work_thread_ptr;
    bool should_stop;
};


#endif
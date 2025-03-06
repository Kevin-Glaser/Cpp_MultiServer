#include "LogUtil/LogUtil.h"
#include "ThreadPool/ThreadPool.h"
#include "ServerUtil/ServerUtil.h"
#include "ConfigUtil/ConfigUtil.h"

#include <chrono>
#include <iomanip>
#include <sstream>


// 生成包含当前日期的文件名
std::string getCurrentDateTimeFilename() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
    localtime_r(&now_c, &now_tm);

    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y_%m_%d") << ".log";
    return oss.str();
}

int main(int argc, char* argv[]) {
    // 线程池和任务队列
//     ThreadPool& thread_pool = ThreadPool::GetInstance(4);
//     Logger& logger = Logger::GetInstance();
//     auto terminalLogHandler = std::make_unique<TerminalLogHandler>();
//     std::string logFileName = getCurrentDateTimeFilename();
//     auto fileLogHandler = std::make_unique<FileLogHandler>(logFileName);
//     logger.AddHandler(std::move(terminalLogHandler));
//     logger.AddHandler(std::move(fileLogHandler));
//     // 添加测试int返回值的任务
//     std::future<int> r1 = thread_pool.submitTask(sum1, 10, 20);
//     thread_pool.printStatus();
// 	std::future<int> r2 = thread_pool.submitTask(sum2, 10, 20, 30);
//     thread_pool.printStatus();
//     std::future<int> r3 = thread_pool.submitTask(sum2, 10, 20, 30);
//     thread_pool.printStatus();
//     std::future<int> r4 = thread_pool.submitTask(sum2, 10, 20, 30);
//     thread_pool.printStatus();
//     std::future<int> r5 = thread_pool.submitTask(sum2, 10, 20, 30);
//     thread_pool.printStatus();
//     std::future<int> r6 = thread_pool.submitTask(sum2, 10, 20, 30);
//     thread_pool.printStatus();
//     std::future<int> r7 = thread_pool.submitTask(sum2, 10, 20, 30);
//     thread_pool.printStatus();
//     //添加成员函数任务,成员函数的返回值获取
//     thread_pool.submitTask(std::bind(&Logger::WriteLog, &logger, "Goodbye", DEBUG));
// 	r1.get();
// 	r2.get();
//     r3.get();
//     r4.get();
//     r5.get();
//    r6.get();
//     r7.get();
//     thread_pool.printStatus();
	

    ConfigManager configer;
    configer.readConfigByKey("/home/demo/Documents/Cpp_MultiServer/Server/config.conf", "DB", "port");
    srand(time(nullptr));
    try {
        ServerUtil server;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred in main: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
    return 0;

}


int sum1(int a, int b)
{
   // std::cout << "Executing sum1 on thread:" << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
	return a + b;
}

int sum2(int a, int b, int c)
{
  //  std::cout << "Executing sum2 on thread:" << std::this_thread::get_id() << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
    return a + b + c;
}

std::string funA(std::string str) {
    std::cout << "Executing funA on thread:" << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return "hello" + str;
}

void funC(int i) {
    std::cout << "Executing funC thread:" << std::this_thread::get_id() << "Executing func" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "funC: " << i << std::endl;
}
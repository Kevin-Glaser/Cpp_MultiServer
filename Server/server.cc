#include "DBUtil/DatabasePoolUtil.h"
#include "LogUtil/LogUtil.h"
#include "ThreadPool/ThreadPool.h"
#include "FtpUtil/ftplib.h"
#include "ServerUtil/ServerUtil.h"
#include "ConfigManager/ConfigManager.h"

#include <chrono>
#include <iomanip>
#include <sstream>



void logCallback(char *str, void *arg, bool out) {
    std::cout << (out ? ">> " : "<< ") << str << std::endl;
}

int InitFtpConn(std::string ipaddr, int port)
{
    ftplib ftp;
    ftp.SetCallbackLogFunction(logCallback);
    std::string host = ipaddr + ":" + std::to_string(port);
    std::cout << host << std::endl;
    int m = ftp.Connect(host.c_str());
    if(m != 1) {
        std:: cout << "Cannot connet " << ipaddr <<ftp.LastResponse() << std::endl;
        return -1;
    }
    ftp.SetConnmode(ftplib::pasv);
    int n = ftp.Login("demo","demo");
    if(n != 1) {
        std:: cout << "Cannot login username demo" << ftp.LastResponse() << std::endl;
        ftp.Quit();
    }
    ftp.Rmdir("/Documents/test_ftp_dir");
    ftp.Mkdir("/Documents/test_ftp_dir");
    ftp.Quit();
    return 0;
}

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

    // 数据库连接池
    // std::string server = readConfig("config.conf", "server");
    // int port = std::atoi(readConfig("config.conf", "port").c_str());
    // InitFtpConn(server, port);
    // std::string url = readConfig("config.conf", "db_url");
    // std::string user = readConfig("config.conf", "db_user");
    // std::string pwd = readConfig("config.conf", "db_pwd");
    // std::string db = readConfig("config.conf", "db_database");
    // DataBasePool& pool = DataBasePool::GetInstance(4);
    // pool.CreateNewConn(url, user, pwd, db);
    // std::shared_ptr<sql::Connection> con = pool.GetConnection(url, user, pwd, db);
    // if (con) {
    //     std::cout << "Got a connection from the pool for " << url << std::endl;
    //     pool.SetTransaction(con, "REPEATABLE READ");
    //     map<string, string> info = {
    //         {"username", "4235231334"},
    //         {"email", "321434"},
    //         {"password", "pas12352523"}
    //     };
    //     pool.InsertData(con, "users", info);
    //     pool.ReleaseConnection(con);
    // } else {
    //     std::cout << "Failed to get a connection from the pool for " << url << std::endl;
    // }
    // // 销毁连接
    //  pool.DestroyConn();


    // 线程池和任务队列
    // ThreadPool& thread_pool = ThreadPool::GetInstance(4);
    // Logger& logger = Logger::GetInstance();
    // auto terminalLogHandler = std::make_unique<TerminalLogHandler>();
    // std::string logFileName = getCurrentDateTimeFilename();
    // auto fileLogHandler = std::make_unique<FileLogHandler>(logFileName);
    // logger.AddHandler(std::move(terminalLogHandler));
    // logger.AddHandler(std::move(fileLogHandler));
    // // 添加测试int返回值的任务
    // std::future<int> r1 = pool.submitTask(sum1, 10, 20);
    //     pool.printStatus();
	// std::future<int> r2 = pool.submitTask(sum2, 10, 20, 30);
    //     pool.printStatus();
    // std::future<int> r3 = pool.submitTask(sum2, 10, 20, 30);
    //     pool.printStatus();
    // std::future<int> r4 = pool.submitTask(sum2, 10, 20, 30);
    //     pool.printStatus();
    // std::future<int> r5 = pool.submitTask(sum2, 10, 20, 30);
    //     pool.printStatus();
    // std::future<int> r6 = pool.submitTask(sum2, 10, 20, 30);
    //     pool.printStatus();
    // std::future<int> r7 = pool.submitTask(sum2, 10, 20, 30);
    //     pool.printStatus();
    // 添加成员函数任务,成员函数的返回值获取
    //thread_pool.submitTask(std::bind(&Logger::WriteLog, &logger, "Goodbye", DEBUG));
	// r1.get();
	// r2.get();
    // r3.get();
    // r4.get();
    // r5.get();
  //  r6.get();
    // r7.get();
    // pool.printStatus();
	

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
}
#include "DBUtil/DatabaseUtil.h"
#include "LogUtil/LogUtil.h"
#include "ThreadPool/ThreadPool.h"

#include "FtpUtil/ftplib.h"

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
    ftp.Mkdir("/Documents/test_ftp_dir");
    ftp.Quit();
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

int main(int argc, char* argv[]) {

    InitFtpConn("123.456.78.109", 12345);

    // ThreadPool& pool = ThreadPool::GetInstance(4);
    // // DataBase& db1 = DataBase::GetInstance("name1", "pwd1");
    // // Logger& logger = Logger::GetInstance();

    // // auto terminalLogHandler = std::make_unique<TerminalLogHandler>();
    // // auto fileLogHandler = std::make_unique<FileLogHandler>("test.log");
    // // logger.AddHandler(std::move(terminalLogHandler));
    // // logger.AddHandler(std::move(fileLogHandler));


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

    // // 添加成员函数任务,成员函数的返回值获取
    // // auto r6 = pool.submitTask(std::bind(&Logger::WriteLog, &logger, "Goodbye", DEBUG));
	// r1.get();
	// r2.get();
    // r3.get();
    // r4.get();
    // r5.get();
    // r6.get();
    // r7.get();

    // pool.printStatus();
	return 0;

}



#include "DBUtil/DatabaseUtil.h"
#include "LogUtil/LogUtil.h"
#include "ThreadPool/ThreadPool.h"

#include "FtpUtil/ftplib.h"

#include <sstream>

#include <mysql/mysql.h>

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

std::string readConfig(const std::string& filename, const std::string& key) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string fileKey = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            if (fileKey == key) {
                return value;
            }
        }
    }

    throw std::runtime_error("Key not found: " + key);
}

// 写入配置文件
void writeConfig(const std::string& filename, const std::string& key, const std::string& value) {
    std::ifstream file(filename);
    std::stringstream buffer;
    bool keyFound = false;

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string fileKey = line.substr(0, pos);
                if (fileKey == key) {
                    buffer << key << "=" << value << "\n";
                    keyFound = true;
                } else {
                    buffer << line << "\n";
                }
            } else {
                buffer << line << "\n";
            }
        }
        file.close();
    }

    if (!keyFound) {
        buffer << key << "=" << value << "\n";
    }

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    outFile << buffer.str();
    outFile.close();
}

// 创建配置文件
void createConfig(const std::string& filename, const std::vector<std::pair<std::string, std::string>>& keyValuePairs) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create file: " + filename);
    }

    for (const auto& pair : keyValuePairs) {
        file << pair.first << "=" << pair.second << "\n";
    }

    file.close();
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

// 添加到DBUtil里
struct DatabaseConnection {
    MYSQL * connection;
    std::string db_name;
};

DatabaseConnection connectToDatabase(const std::string& url,
                                    const std::string& username,
                                    const std::string& password,
                                    const std::string& dbName = "") {
    MYSQL *connection = mysql_init(nullptr);

    if (connection == nullptr) {
        throw std::runtime_error("Failed to initialize MySQL connection.");
    }

    MYSQL *conn = nullptr;
    if (url == "localhost" || url.empty()) {
        conn = mysql_real_connect(connection, nullptr, username.c_str(), password.c_str(),
                                  dbName.c_str(), 0, nullptr, 0);
    } else {
        conn = mysql_real_connect(connection, url.c_str(), username.c_str(), password.c_str(),
                                  dbName.c_str(), 0, nullptr, 0);
    }

    if (conn == nullptr) {
        std::string error = "Failed to connect to MySQL database: " + std::string(mysql_error(connection));
        mysql_close(connection);
        std::cout << error << std::endl;
    }

    return {connection, dbName};
}

// 改成类的智能指针
void disconnectFromDatabase(DatabaseConnection& db) {
    mysql_close(db.connection);
}


int main(int argc, char* argv[]) {

    std::string server = readConfig("config.conf", "server");
    int port = std::atoi(readConfig("config.conf", "port").c_str());
    InitFtpConn(server, port);


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



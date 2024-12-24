#include <iostream>

#include <string>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>

class Database
{
private:
    std::shared_ptr<sql::Connection> p_conn;

public:
    Database() {
        std::cout << "Database()" << std::endl;
    }

    bool ConnectMySQL(const std::string& host, const std::string& user, const std::string& password, const std::string& database);

    inline std::shared_ptr<sql::Connection> GetConnection() const {
        return p_conn;
    }

    bool CloseConnection();

    virtual ~Database() {
        std::cout << "~Database()" << std::endl;
    }
};
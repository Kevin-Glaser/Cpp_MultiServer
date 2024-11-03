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

    bool ConnectMySQL(const std::string& host, const std::string& user, const std::string& password, const std::string& database) {
        try {
            sql::Driver* driver = get_driver_instance();
            p_conn.reset(driver->connect(host, user, password));
            p_conn->setSchema(database);
            std::cout << "Connected to database successfully" << std::endl;
            return true;
        } catch (sql::SQLException& e) {
            std::cout << "SQL Exception: " << e.what() << std::endl;
            std::cout << "MySQL Error Code: " << e.getErrorCode() << std::endl;
            std::cout << "SQL State: " << e.getSQLState() << std::endl;
            return false;
        }
    }

    std::shared_ptr<sql::Connection> GetConnection() const {
        return p_conn;
    }

    bool CloseConnection() {
        if(p_conn) {
            p_conn->close();
            p_conn.reset();
            return true;
        }
        return false;
    }
    virtual ~Database() {
        std::cout << "~Database()" << std::endl;
    }
};
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
    std::shared_ptr<sql::Driver> p_driver;
};

bool ConnectMySQL(const std::string& host, const std::string& user, const std::string& password, const std::string& database, sql::Connection* conn) {
    try {
        sql::Driver* driver = get_driver_instance();
        conn = driver->connect(host, user, password);

        conn->setSchema(database);

        std::cout << "Connected to database successfully" << std::endl;

        delete conn;
        return true;
    }
    catch(sql::SQLException& e) {
        std::cout << "SQL Exception:" << e.what() << std::endl;
        std::cout << "MySQL Error Code: " << e.getErrorCode() << std::endl;
        std::cout << "SQL State: " << e.getSQLState() << std::endl;
        return false;
    }
    return true;
}

bool CloseMySQL(sql::Connection* conn) {
    if(conn) {
        try {
            conn->close();
            delete conn;
            std::cout << "Database connetion closed suuccessfully" << std::endl;
        } catch(sql::SQLException& e) {
            std::cout << "Error close database connection:" << e.what() << std::endl;
        }
    }
}
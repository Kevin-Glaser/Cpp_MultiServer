#include "DatabaseUtil.h"

bool Database::ConnectMySQL(const std::string &host, const std::string &user, const std::string &password, const std::string &database) {
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

bool Database::CloseConnection(){
    if(p_conn) {
        p_conn->close();
        p_conn.reset();
        return true;
    }
    return false;
}

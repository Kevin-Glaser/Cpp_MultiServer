#include "DatabasePoolUtil.h"

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>

void DataBasePool::CreateNewConn(const std::string& host, const std::string& user, const std::string& password, const std::string& database) {
    p_db_impl->CreateConntoMySQL(host, user, password, database);
}

bool DataBasePool::DestroyConn() {
    return p_db_impl->DestroyConn();
}

std::shared_ptr<sql::Connection> DataBasePool::GetConnection(const std::string& host, const std::string& user, const std::string& password, const std::string& database) {
    return p_db_impl->GetConnection(host, user, password, database);
}

void DataBasePool::ReleaseConnection(std::shared_ptr<sql::Connection> conn) {
    p_db_impl->ReleaseConnection(conn);
}

bool DataBasePool::InsertData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& values)
{
    return p_db_impl->InsertData(conn, table, values);
}

bool DataBasePool::SetTransaction(std::shared_ptr<sql::Connection> &conn, const std::string& level)
{
    return p_db_impl->SetTransaction(conn, level);
}

void DataBasePool::SelectData(std::shared_ptr<sql::Connection>& conn, const std::string &table, const std::map<std::string, std::string> &conditions)
{
    p_db_impl->SelectData(conn, table, conditions);
}

void DataBasePool::UpdateData(std::shared_ptr<sql::Connection> &conn, const std::string &table, const std::map<std::string, std::string> &values, const std::map<std::string, std::string> &conditions)
{
    p_db_impl->UpdateData(conn, table, values, conditions);
}

void DataBasePool::DeleteData(std::shared_ptr<sql::Connection> &conn, const std::string &table, const std::map<std::string, std::string> &conditions)
{
    p_db_impl->DeleteData(conn, table, conditions);
}

void DataBasePool::DataBasePoolImpl::CreateConntoMySQL(const std::string &host, const std::string &user, const std::string &password, const std::string &database) {
    auto db = std::make_shared<Database>();
    if (db->ConnectMySQL(host, user, password, database)) {
        p_db_list.emplace_back(db);

        try {
            auto conn = db->GetConnection();
            if (conn) {
                std::lock_guard<std::mutex> lock(mutex);
                available_conns[host + user + password + database].push_back(conn);
                std::cout << "Connection added to pool for " << host << std::endl;
            } else {
                std::cerr << "Failed to get a valid connection" << std::endl;
            }
        } 
        catch (sql::SQLException& e) {
            std::cout << "Exception:"<< e.what() << std::endl;
        }
        
    } else {
        std::cerr << "Failed to create connection" << std::endl;
    }
}

bool DataBasePool::DataBasePoolImpl::DestroyConn() {
    for (auto& db : p_db_list) {
        if (db) {
            db->CloseConnection();
        }
    }
    p_db_list.clear();
    available_conns.clear();
    return true;
}

std::shared_ptr<sql::Connection> DataBasePool::DataBasePoolImpl::GetConnection(const std::string &host, const std::string &user, const std::string &password, const std::string &database) {
    std::lock_guard<std::mutex> lock(mutex);
    auto key = host + user + password + database;
    if (available_conns.find(key) != available_conns.end() && !available_conns[key].empty()) {
        auto conn = available_conns[key].back();
        available_conns[key].pop_back();
        return conn;
    }
    return nullptr;
}

void DataBasePool::DataBasePoolImpl::ReleaseConnection(std::shared_ptr<sql::Connection> conn)
{
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& [key, conns] : available_conns) {
        for (auto& c : conns) {
            if (c.get() == conn.get()) {
                conns.push_back(conn);
                return;
            }
        }
    }
}

bool DataBasePool::DataBasePoolImpl::InsertData(std::shared_ptr<sql::Connection> &conn, const std::string& table, const std::map<std::string, std::string>& values)
{
    try {
        std::stringstream sql;
        sql << "INSERT INTO " << table << " (";
        bool first = true;
        for (const auto& pair : values) {
            if (!first) {
                sql << ", ";
            }
            sql << pair.first;
            first = false;
        }
        sql << ") VALUES (";

        first = true;
        for (const auto& pair : values) {
            if (!first) {
                sql << ", ";
            }
            sql << "?";
            first = false;
        }
        sql << ")";
        // 准备并执行插入语句
        sql::PreparedStatement* pstmt = conn->prepareStatement(sql.str());
        int paramIndex = 1;
        for (const auto& value : values) {
            pstmt->setString(paramIndex++, value.second);
        }
        pstmt->execute();

        // 查询数据以验证插入是否成功
        sql::Statement* stmt = conn->createStatement();
        sql::ResultSet* res = stmt->executeQuery("SELECT * FROM " + table);
        std::cout << "Inserted data in table " << table << ":" << std::endl;
        while (res->next()) {
            for (const auto& pair : values) {
                std::cout << pair.first << ": " << res->getString(pair.first) << ", ";
            }
            std::cout << std::endl;
        }

        delete stmt;
        delete pstmt;
        return true;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
        std::cout << "MySQL Error Code: " << e.getErrorCode() << std::endl;
        std::cout << "SQL State: " << e.getSQLState() << std::endl;
        return false;
    }
}

bool DataBasePool::DataBasePoolImpl::SetTransaction(std::shared_ptr<sql::Connection> &conn, const std::string& level)
{
    try {
        conn->setAutoCommit(false); // 关闭自动提交
        sql::Statement* stmt = conn->createStatement();
        stmt->execute("SET SESSION TRANSACTION ISOLATION LEVEL " + level);
        stmt->execute("START TRANSACTION");
        delete stmt;
        return true;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
        std::cout << "MySQL Error Code: " << e.getErrorCode() << std::endl;
        std::cout << "SQL State: " << e.getSQLState() << std::endl;
        return false;
    } 
}

void DataBasePool::DataBasePoolImpl::SelectData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& conditions) {
    try {
        // 构建查询语句
        std::stringstream sql;
        sql << "SELECT * FROM " << table;
        if (!conditions.empty()) {
            sql << " WHERE ";
            bool first = true;
            for (const auto& pair : conditions) {
                if (!first) {
                    sql << " AND ";
                }
                sql << pair.first << " = ?";
                first = false;
            }
        }

        // 准备并执行查询语句
        sql::PreparedStatement* pstmt = conn->prepareStatement(sql.str());
        int paramIndex = 1;
        for (const auto& condition : conditions) {
            pstmt->setString(paramIndex++, condition.second);
        }
        sql::ResultSet* res = pstmt->executeQuery();

        std::cout << "Selected data from table " << table << ":" << std::endl;
        while (res->next()) {
            for (const auto& pair : conditions) {
                std::cout << pair.first << ": " << res->getString(pair.first) << ", ";
            }
            std::cout << std::endl;
        }

        delete res;
        delete pstmt;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
        std::cout << "MySQL Error Code: " << e.getErrorCode() << std::endl;
        std::cout << "SQL State: " << e.getSQLState() << std::endl;
    }
}

void DataBasePool::DataBasePoolImpl::UpdateData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& values, const std::map<std::string, std::string>& conditions) {
    try {
        // 构建更新语句
        std::stringstream sql;
        sql << "UPDATE " << table << " SET ";
        bool first = true;
        for (const auto& pair : values) {
            if (!first) {
                sql << ", ";
            }
            sql << pair.first << " = ?";
            first = false;
        }
        sql << " WHERE ";
        first = true;
        for (const auto& pair : conditions) {
            if (!first) {
                sql << " AND ";
            }
            sql << pair.first << " = ?";
            first = false;
        }

        // 准备并执行更新语句
        sql::PreparedStatement* pstmt = conn->prepareStatement(sql.str());
        int paramIndex = 1;
        for (const auto& value : values) {
            pstmt->setString(paramIndex++, value.second);
        }
        for (const auto& condition : conditions) {
            pstmt->setString(paramIndex++, condition.second);
        }
        pstmt->execute();

        delete pstmt;

        // 查询数据以验证更新是否成功
        sql::Statement* stmt = conn->createStatement();
        sql::ResultSet* res = stmt->executeQuery("SELECT * FROM " + table);
        std::cout << "Updated data in table " << table << ":" << std::endl;
        while (res->next()) {
            for (const auto& pair : values) {
                std::cout << pair.first << ": " << res->getString(pair.first) << ", ";
            }
            std::cout << std::endl;
        }
        delete res;
        delete stmt;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
        std::cout << "MySQL Error Code: " << e.getErrorCode() << std::endl;
        std::cout << "SQL State: " << e.getSQLState() << std::endl;
    }
}

void DataBasePool::DataBasePoolImpl::DeleteData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& conditions) {
    try {
        // 构建删除语句
        std::stringstream sql;
        sql << "DELETE FROM " << table << " WHERE ";
        bool first = true;
        for (const auto& pair : conditions) {
            if (!first) {
                sql << " AND ";
            }
            sql << pair.first << " = ?";
            first = false;
        }

        // 准备并执行删除语句
        sql::PreparedStatement* pstmt = conn->prepareStatement(sql.str());
        int paramIndex = 1;
        for (const auto& condition : conditions) {
            pstmt->setString(paramIndex++, condition.second);
        }
        pstmt->execute();

        delete pstmt;

        // 查询数据以验证删除是否成功
        sql::Statement* stmt = conn->createStatement();
        sql::ResultSet* res = stmt->executeQuery("SELECT * FROM " + table);
        std::cout << "Data after deletion in table " << table << ":" <<std::endl;
        while (res->next()) {
            for (const auto& pair : conditions) {
                std::cout << pair.first << ": " << res->getString(pair.first) << ", ";
            }
            std::cout << std::endl;
        }
        delete res;
        delete stmt;
    } catch (sql::SQLException& e) {
        std::cout << "SQL Error: " << e.what() << std::endl;
        std::cout << "MySQL Error Code: " << e.getErrorCode() << std::endl;
        std::cout << "SQL State: " << e.getSQLState() << std::endl;
    }
}
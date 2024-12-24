#ifndef __DATABASE_UTILS_H__
#define __DATABASE_UTILS_H__

#include <iostream>
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>

#include "SingletonBase/Singleton.h"
#include "DatabaseUtil.h"

class DataBasePool final : public Singleton<DataBasePool> {
public:
    void CreateNewConn(const std::string& host, const std::string& user, const std::string& password, const std::string& database);
    bool DestroyConn();
    std::shared_ptr<sql::Connection> GetConnection(const std::string& host, const std::string& user, const std::string& password, const std::string& database);
    void ReleaseConnection(std::shared_ptr<sql::Connection> conn);
    bool InsertData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& values);
    bool SetTransaction(std::shared_ptr<sql::Connection>& conn, const std::string& level);
    void SelectData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& conditions);
    void UpdateData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& values, const std::map<std::string, std::string>& conditions);
    void DeleteData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& conditions);

private:
    explicit DataBasePool(int pool_size) 
        : p_db_impl(std::make_unique<DataBasePoolImpl>(pool_size)){
        std::cout <<"DataBasePool" << std::endl;
    }
    ~DataBasePool() override {std::cout << "~DataBasePool" << std::endl;}

private:
    class DataBasePoolImpl;
    std::unique_ptr<DataBasePoolImpl> p_db_impl;
    friend class Singleton<DataBasePool>;
};

class DataBasePool::DataBasePoolImpl {
public:
    DataBasePoolImpl(int pool_size) {
            p_db_list.reserve(pool_size);
            available_conns.reserve(pool_size);
            std::cout << "DataBasePoolImpl" << std::endl;
    }

    virtual ~DataBasePoolImpl() {std::cout << "~DataBasePoolImpl" << std::endl;}

    void CreateConntoMySQL(const std::string& host, const std::string& user, const std::string& password, const std::string& database);

    bool DestroyConn();

    std::shared_ptr<sql::Connection> GetConnection(const std::string& host, const std::string& user, const std::string& password, const std::string& database);

    void ReleaseConnection(std::shared_ptr<sql::Connection> conn);

    bool InsertData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& values);

    bool SetTransaction(std::shared_ptr<sql::Connection>& conn, const std::string& level);

    void SelectData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& conditions);

    void UpdateData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& values, const std::map<std::string, std::string>& conditions);

    void DeleteData(std::shared_ptr<sql::Connection>& conn, const std::string& table, const std::map<std::string, std::string>& conditions);
private:
    std::vector<std::shared_ptr<Database>> p_db_list;
    std::unordered_map<std::string, std::vector<std::shared_ptr<sql::Connection>>> available_conns;
    std::mutex mutex;
};

#endif
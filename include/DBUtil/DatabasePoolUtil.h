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

    void CreateConntoMySQL(const std::string& host, const std::string& user, const std::string& password, const std::string& database) {
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
            catch (sql::SQLException e) {
                std::cout << "Exception:"<< e.what() << std::endl;
            }
            
        } else {
            std::cerr << "Failed to create connection" << std::endl;
        }
    }

    bool DestroyConn() {
        for (auto& db : p_db_list) {
            if (db) {
                db->CloseConnection();
            }
        }
        p_db_list.clear();
        available_conns.clear();
        return true;
    }

    std::shared_ptr<sql::Connection> GetConnection(const std::string& host, const std::string& user, const std::string& password, const std::string& database) {
        std::lock_guard<std::mutex> lock(mutex);
        auto key = host + user + password + database;
        if (available_conns.find(key) != available_conns.end() && !available_conns[key].empty()) {
            auto conn = available_conns[key].back();
            available_conns[key].pop_back();
            return conn;
        }
        return nullptr;
    }

    void ReleaseConnection(std::shared_ptr<sql::Connection> conn) {
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
private:
    std::vector<std::shared_ptr<Database>> p_db_list;
    std::unordered_map<std::string, std::vector<std::shared_ptr<sql::Connection>>> available_conns;
    std::mutex mutex;
};

#endif
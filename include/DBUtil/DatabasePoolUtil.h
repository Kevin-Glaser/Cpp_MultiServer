#ifndef __DATABASE_UTILS_H__
#define __DATABASE_UTILS_H__

#include <iostream>
#include <memory>
#include <vector>

#include "SingletonBase/Singleton.h"
#include "DatabaseUtil.h"

class DataBasePool final : public Singleton<DataBasePool> {
public:
    void connect();
    void printInfo();
private:
    class DataBasePoolImpl;
    std::unique_ptr<DataBasePoolImpl> p_db;
    friend class Singleton<DataBasePool>;

    DataBasePool(const std::string& username, const std::string& pwd) 
        : p_db(std::make_unique<DataBasePoolImpl>(username, pwd)){
        std::cout <<"DataBase" << std::endl;
    }
    ~DataBasePool() override {std::cout << "~DataBase" << std::endl;}
};

class DataBasePool::DataBasePoolImpl {
public:
    DataBasePoolImpl(const std::string& username, const std::string& pwd) 
        : s_username(username), s_pwd(pwd) {
            b_isConnected = true;
            std::cout << "DataBasePoolImpl" << std::endl;
    }

    virtual ~DataBasePoolImpl() {std::cout << "~DataBasePoolImpl" << std::endl;}

    void print(){
    }
private:
    std::string s_username;
    std::string s_pwd;
    bool b_isConnected;
    std::string s_sqlAddr;
    std::vector<std::shared_ptr<Database>> p_db;

};

#endif
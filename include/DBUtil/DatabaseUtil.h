#ifndef __DATABASE_UTILS_H__
#define __DATABASE_UTILS_H__

#include <iostream>
#include <memory>

#include "SingletonBase/Singleton.h"
class DataBase final : public Singleton<DataBase> {
public:
    void connect();
    void printInfo();
private:
    class DataBaseImpl;
    std::unique_ptr<DataBaseImpl> p_db;
    friend class Singleton<DataBase>;

    DataBase(const std::string& username, const std::string& pwd) 
        : p_db(std::make_unique<DataBaseImpl>(username, pwd)){
        std::cout <<"DataBase" << std::endl;
    }
    ~DataBase() override {std::cout << "~DataBase" << std::endl;}
};

class DataBase::DataBaseImpl {
public:
    DataBaseImpl(const std::string& username, const std::string& pwd) 
        : s_username(username), s_pwd(pwd) {
            b_isConnected = true;
            std::cout << "DataBaseImpl" << std::endl;
    }

    virtual ~DataBaseImpl() {std::cout << "~DataBaseImpl" << std::endl;}

    void print(){
    }
private:
    std::string s_username;
    std::string s_pwd;
    bool b_isConnected;
    std::string s_sqlAddr;
};

#endif
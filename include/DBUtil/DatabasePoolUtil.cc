#include "DatabasePoolUtil.h"

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
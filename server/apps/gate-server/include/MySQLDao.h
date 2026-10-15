#ifndef MYSQLDAO_H
#define MYSQLDAO_H

#include "ConfigManager.h"
#include "utils.h"
#include <thread>
#include <atomic>
#include <queue>
#include <chrono>
#include <memory>
#include <condition_variable>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/driver.h>
#include <jdbc/cppconn/connection.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/exception.h>

class SQLConnection {
public:
    SQLConnection(sql::Connection *con, int64_t lasttime);
    std::unique_ptr<sql::Connection> con_;
    int64_t last_oper_time;

};

enum class EmailCheckResult {
    NotFound,
    Exists,
    QueryFailed
};

enum class LoginCheckResult {
    Success,
    InvalidCredentials,
    QueryFailed
};

class MySQLPool{
public:
    MySQLPool(const std::string& url,
        const std::string user,
        const std::string pass,
        const std::string schema,
        size_t poolSize
    );

    ~MySQLPool();

    std::unique_ptr<SQLConnection> getConnection();

    void checkConnection();

    void returnConnection(std::unique_ptr<SQLConnection> con);

    void Close();

private:


    std::string url_;
    std::string user_;
    std::string pass_;
    std::string schema_;// 要使用的数据库
    int poolSize_;
    std::queue<std::unique_ptr<SQLConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> b_stop_;
    std::thread check_thread_;
};

class MySQLDAO {
public:
    MySQLDAO();
    ~MySQLDAO();
    int RegUser(const std::string & name, const std::string& email, const std::string& pwd);
    EmailCheckResult CheckEmailExists(const std::string& email);
    LoginCheckResult CheckLogin(const std::string& name, const std::string& pwd, int& uid);
    // bool CheckEmail(const std::string& name, const std::string& email);
    // bool UpdatePwd(const std::string& name, const std::string& newpwd);
    // bool CheckPwd(const std::string &name, const std::string & pwd);

private:
    std::unique_ptr<MySQLPool> pool_;

};

#endif /* MYSQLDAO_H */

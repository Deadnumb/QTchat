#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H

#include "MySQLDao.h"
#include "Singleton.h"
#include "global.h"

class MySQLManager :public Singleton<MySQLManager>{
    friend class Singleton<MySQLManager>;
public:
    ~MySQLManager();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    EmailCheckResult CheckEmailExists(const std::string& email);
    LoginCheckResult CheckLogin(const std::string& name,
                                const std::string& pwd,
                                int& uid);

private:
    MySQLManager();
    MySQLDAO dao_;
};

#endif /* MYSQLMANAGER_H */

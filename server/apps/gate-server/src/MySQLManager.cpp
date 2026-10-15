#include "MySQLManager.h"

MySQLManager::~MySQLManager() {

}

int MySQLManager::RegUser(const std::string &name, const std::string &email,
                          const std::string &pwd) {
    return dao_.RegUser(name, email, pwd);
}

EmailCheckResult MySQLManager::CheckEmailExists(const std::string& email) {
    return dao_.CheckEmailExists(email);
}

LoginCheckResult MySQLManager::CheckLogin(const std::string& name,
                                          const std::string& pwd,
                                          int& uid) {
    return dao_.CheckLogin(name, pwd, uid);
}

MySQLManager::MySQLManager() {

}

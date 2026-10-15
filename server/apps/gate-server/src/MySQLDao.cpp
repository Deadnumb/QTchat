#include "MySQLDao.h"

// 初始化连接池，里面预置poolSize个连接
MySQLPool::MySQLPool(const std::string &url,
    const std::string user,
    const std::string pass,
    const std::string schema,
    size_t poolSize) : url_(url), user_(user), pass_(pass),schema_(schema), b_stop_(false)
{
    try {
        for(size_t i = 0; i < poolSize; i++) {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            auto *con = driver->connect(url_, user_, pass_);
            // 设置数据库名字
            con->setSchema(schema_);
            auto currentTime = std::chrono::system_clock::now().time_since_epoch();
            long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
            pool_.push(std::make_unique<SQLConnection>(con, timestamp));
        }

        check_thread_ = std::thread([this](){
            while (!b_stop_) {
                checkConnection();

                std::unique_lock<std::mutex> lock(mutex_);
                cond_.wait_for(
                    lock,
                    std::chrono::seconds(5),
                    [this] { return b_stop_.load(); });
            }
        });
    } catch (std::exception &e) {
        std::cerr << "MySQL Pool Error:" << e.what() << std::endl;
        throw ;
    }


}

// 清理所有的连接
MySQLPool::~MySQLPool()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        pool_.pop();
    }
}

// 关闭连接
void MySQLPool::Close() {
    if (b_stop_.exchange(true)) {
        return;
    }

    cond_.notify_all();
    if (check_thread_.joinable()) {
        check_thread_.join();
    }
}

// 用于检查连接，如果连接上次操作间隔太久，就发一次心跳，如果连接丢失就重新创建
void MySQLPool::checkConnection() {
    // 遍历连接池发心跳。
    std::lock_guard<std::mutex> guard(mutex_);
    auto currentTime = std::chrono::system_clock::now().time_since_epoch();
    long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
    for(size_t i = 0; i < poolSize_; i++) {
        auto con = std::move(pool_.front());
        pool_.pop();
        Defer defer([this, &con]() {
            pool_.push(std::move(con));
        });

        if(timestamp - con->last_oper_time < 5) {
            continue;
        }

        try {
            std::unique_ptr<sql::Statement> stmt(con->con_->createStatement());
            // 如果拿到了连接，就执行一下SELECT 1发一个心跳。
            stmt->executeQuery("SELECT 1");
            con->last_oper_time = timestamp;
            std::cout << "execute timer alive query, cur is " << timestamp << std::endl;
        } catch (sql::SQLException &e) {
            // 如果连接没了，创建一个新连接替换。
            std::cerr << "Error Keeping connection alive:" << std::endl;
            auto driver = sql::mysql::get_mysql_driver_instance();
            auto *newcon = driver->connect(url_, user_, pass_);
            newcon->setSchema(schema_);
            con->con_.reset(newcon);
            con->last_oper_time = timestamp;
        }
    }
}

// 获取连接
std::unique_ptr<SQLConnection> MySQLPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]()->int{
        if(b_stop_) {
            return true;
        }
        return !pool_.empty();
    });
    if(b_stop_) {
        return nullptr;
    }
    std::unique_ptr<SQLConnection> con(std::move(pool_.front()));
    pool_.pop();
    return con;
}

// 归还连接
void MySQLPool::returnConnection(std::unique_ptr<SQLConnection> con) {
    if (con == nullptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        pool_.push(std::move(con));
    }
    cond_.notify_one();
}

// 封装的连接类，包含上次操作时刻，用于心跳保活。
SQLConnection::SQLConnection(sql::Connection *con, int64_t lasttime) :con_(con), last_oper_time(lasttime)
{}

MySQLDAO::MySQLDAO() {
    auto &cfg = ConfigManager::get_instance();
    const auto &host = cfg["MySQL"]["Host"];
    const auto &port = cfg["MySQL"]["Port"];
    const auto &pwd = cfg["MySQL"]["Password"];
    const auto &schema = cfg["MySQL"]["Schema"];
    const auto &user = cfg["MySQL"]["User"];
    pool_.reset(new MySQLPool(host + ":" + port, user, pwd, schema, 10));

}

MySQLDAO::~MySQLDAO()
{
    pool_->Close();
}

int MySQLDAO::RegUser(const std::string &name, const std::string &email,
const std::string &pwd) {
    auto con = pool_->getConnection();
    try {
        if(con == nullptr) {
            return -1;
        }
        // 准备调用存储过程
        std::unique_ptr<sql::PreparedStatement> stmt(con->con_->prepareStatement("CALL reg_user(?,?,?,@result)"));
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);

        // 执行存储过程
        stmt->execute();
        // 如果存储过程设置了会话变量或者其它方式输出参数，这里可以执行SELECT查询来获取它们
        // 例如存储过程设置了一个会话变量@result，可以这样获取：
        std::unique_ptr<sql::Statement> stmtResult(con->con_->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        // 如果有结果
        if(res->next()) {
            int result = res->getInt("result");
            std::cout << "Result:" << result << std::endl;
            pool_->returnConnection(std::move(con));
            return result;
        }
        pool_->returnConnection(std::move(con));
        return -1;

    } catch (sql::SQLException &e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << "(MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << ")" << std::endl;
        return -1;
    }
}

EmailCheckResult MySQLDAO::CheckEmailExists(const std::string& email) {
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return EmailCheckResult::QueryFailed;
    }

    try {
        std::unique_ptr<sql::PreparedStatement> stmt(
            con->con_->prepareStatement(
                "SELECT 1 FROM `user` WHERE `email` = ? LIMIT 1"));
        stmt->setString(1, email);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        const auto result = res->next()
            ? EmailCheckResult::Exists
            : EmailCheckResult::NotFound;
        pool_->returnConnection(std::move(con));
        return result;
    } catch (sql::SQLException &e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException while checking email: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << ")" << std::endl;
        return EmailCheckResult::QueryFailed;
    }
}

LoginCheckResult MySQLDAO::CheckLogin(const std::string &name,
                                      const std::string &pwd, int &uid)
{
    uid = 0;

    auto con = pool_->getConnection();
    if (con == nullptr) {
        return LoginCheckResult::QueryFailed;
    }

    try {
        std::unique_ptr<sql::PreparedStatement> stmt(
            con->con_->prepareStatement(
                "SELECT uid FROM `user` "
                "WHERE `name` = ? AND `pwd` = ? LIMIT 1"));
        stmt->setString(1, name);
        stmt->setString(2, pwd);

        // 执行查询并检查结果
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if (!res->next()) {
            pool_->returnConnection(std::move(con));
            return LoginCheckResult::InvalidCredentials;
        }

        uid = res->getInt("uid");
        pool_->returnConnection(std::move(con));
        return LoginCheckResult::Success;
    } catch (const sql::SQLException &e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException while checking login: "
                  << e.what()
                  << " (MySQL error code: " << e.getErrorCode()
                  << ", SQLState: " << e.getSQLState() << ")"
                  << std::endl;
        return LoginCheckResult::QueryFailed;
    }
}

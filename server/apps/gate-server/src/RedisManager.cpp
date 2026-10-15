#include "RedisManager.h"
#include "ConfigManager.h"

sw::redis::Redis *RedisManager::operator->() {
    if (!redis_) {
        throw std::runtime_error("Redis not initialized");
    }
    return redis_.get();
}

RedisManager::RedisManager()
{
    try {
        // 初始化连接池参数
        auto& gCfgMgr = ConfigManager::get_instance();
        auto host = gCfgMgr["Redis"]["Host"];
        auto port = gCfgMgr["Redis"]["Port"];
        auto password = gCfgMgr["Redis"]["Password"];

        conn_opts_.host = host;
        conn_opts_.port = std::stoi(port);
        conn_opts_.password = password;
        conn_opts_.db = 0;
        conn_opts_.socket_timeout = std::chrono::milliseconds(200);

        pool_opts_.size = 10;
        pool_opts_.wait_timeout = std::chrono::milliseconds(100);

        redis_ = std::make_unique<sw::redis::Redis>(conn_opts_, pool_opts_);



    } catch (std::exception &e) {
        std::cerr << "Redis Error: " << e.what() << std::endl;
        throw;
    }
}

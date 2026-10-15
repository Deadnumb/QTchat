#ifndef REDISMANAGER_H
#define REDISMANAGER_H

#include <sw/redis++/redis++.h>
#include "Singleton.h"

class RedisManager: public Singleton<RedisManager>  {
    friend Singleton<RedisManager>;
public:
    sw::redis::Redis* operator->();

private:
    // 初始化的时候读取配置
    RedisManager();


    // 连接池参数
    sw::redis::ConnectionOptions conn_opts_;
    sw::redis::ConnectionPoolOptions pool_opts_;
    std::unique_ptr<sw::redis::Redis> redis_;
};

#endif /* REDISMANAGER_H */

#ifndef STATUSGRPCCLIENT_H
#define STATUSGRPCCLIENT_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include "ConfigManager.h"
#include "Singleton.h"
#include "global.h"
#include "message.grpc.pb.h"

class StatusConPool {
public:
    StatusConPool(std::size_t pool_size, std::string host, std::string port);
    ~StatusConPool();

    std::unique_ptr<message::StatusService::Stub> getConnection();
    void returnConnection(std::unique_ptr<message::StatusService::Stub> connection);
    void Close();

private:
    std::atomic<bool> stopped_;
    std::string endpoint_;
    std::queue<std::unique_ptr<message::StatusService::Stub>> connections_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

class StatusGrpcClient : public Singleton<StatusGrpcClient> {
    friend class Singleton<StatusGrpcClient>;

public:
    message::GetChatServerRsp GetChatServer(int uid);

private:
    StatusGrpcClient();
    std::unique_ptr<StatusConPool> pool_;
};

#endif

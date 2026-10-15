#ifndef VerifyGRPCCLIENT_H
#define VerifyGRPCCLIENT_H

#include <grpcpp/grpcpp.h>

#include "Singleton.h"
#include "global.h"
#include "ConfigManager.h"
#include "message.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::GetVerifyReq;
using message::GetVerifyRsp;
using message::VerifyService;

class RPConPool {
public:
    RPConPool(size_t poolSize, std::string host, std::string port);
    ~RPConPool();
    std::unique_ptr<VerifyService::Stub> getConnection();
    void returnConnection(std::unique_ptr<VerifyService::Stub> context);
    void Close();
private:
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string port_;
    std::queue<std::unique_ptr<VerifyService::Stub>> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
};



class VerifyGrpcClient : public Singleton<VerifyGrpcClient> {
    friend class Singleton<VerifyGrpcClient>;

  public:
    GetVerifyRsp GetVerifyCode(std::string email);

  private:
    VerifyGrpcClient();
    std::unique_ptr<VerifyService::Stub> stub_;
	std::unique_ptr<RPConPool> pool_;
};

#endif // VerifyGRPCCLIENT_H

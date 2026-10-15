#ifndef STATUSSERVICEIMPL_H
#define STATUSSERVICEIMPL_H

#include <atomic>
#include <cstddef>
#include <grpcpp/grpcpp.h>
#include <string>
#include <vector>

#include "message.grpc.pb.h"

struct ChatServerEndpoint {
    std::string host;
    std::string port;
};

class StatusServiceImpl final : public message::StatusService::Service {
public:
    explicit StatusServiceImpl(const std::string& config_path);

    grpc::Status GetChatServer(
        grpc::ServerContext* context,
        const message::GetChatServerReq* request,
        message::GetChatServerRsp* reply) override;

private:
    std::vector<ChatServerEndpoint> chat_servers_;
    std::atomic<std::size_t> next_server_{0};
};

#endif

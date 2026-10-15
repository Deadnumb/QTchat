#include "StatusGrpcClient.h"

#include <stdexcept>
#include <utility>

StatusConPool::StatusConPool(std::size_t pool_size, std::string host, std::string port)
    : stopped_(false), endpoint_(std::move(host) + ":" + std::move(port)) {
    if (endpoint_ == ":") {
        throw std::runtime_error("StatusServer endpoint is not configured");
    }

    for (std::size_t i = 0; i < pool_size; ++i) {
        auto channel = grpc::CreateChannel(
            endpoint_, grpc::InsecureChannelCredentials());
        connections_.push(message::StatusService::NewStub(channel));
    }
}

StatusConPool::~StatusConPool() {
    Close();
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) {
        connections_.pop();
    }
}

std::unique_ptr<message::StatusService::Stub> StatusConPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] {
        return stopped_.load() || !connections_.empty();
    });
    if (stopped_ || connections_.empty()) {
        return nullptr;
    }

    auto connection = std::move(connections_.front());
    connections_.pop();
    return connection;
}

void StatusConPool::returnConnection(
    std::unique_ptr<message::StatusService::Stub> connection) {
    if (!connection) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_) {
            return;
        }
        connections_.push(std::move(connection));
    }
    condition_.notify_one();
}

void StatusConPool::Close() {
    if (!stopped_.exchange(true)) {
        condition_.notify_all();
    }
}

message::GetChatServerRsp StatusGrpcClient::GetChatServer(int uid) {
    message::GetChatServerReq request;
    request.set_uid(uid);
    message::GetChatServerRsp reply;

    auto stub = pool_->getConnection();
    if (!stub) {
        reply.set_error(ErrorCode::RPCFailed);
        return reply;
    }

    grpc::ClientContext context;
    const auto status = stub->GetChatServer(&context, request, &reply);
    pool_->returnConnection(std::move(stub));
    if (!status.ok()) {
        reply.Clear();
        reply.set_error(ErrorCode::RPCFailed);
    }
    return reply;
}

StatusGrpcClient::StatusGrpcClient() {
    auto& config = ConfigManager::get_instance();
    const auto host = config["StatusServer"]["Host"];
    const auto port = config["StatusServer"]["Port"];
    pool_ = std::make_unique<StatusConPool>(5, host, port);
}

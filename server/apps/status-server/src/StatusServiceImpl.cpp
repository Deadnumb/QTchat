#include "StatusServiceImpl.h"

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {

std::string GenerateToken() {
    static thread_local std::mt19937_64 engine(std::random_device{}());
    std::uniform_int_distribution<unsigned int> distribution(0, 15);
    std::ostringstream token;
    for (int i = 0; i < 32; ++i) {
        token << std::hex << distribution(engine);
    }
    return token.str();
}

void AddServer(const boost::property_tree::ptree& config,
               const std::string& section,
               std::vector<ChatServerEndpoint>& servers) {
    const auto host = config.get<std::string>(section + ".Host", "");
    const auto port = config.get<std::string>(section + ".Port", "");
    if (!host.empty() && !port.empty()) {
        servers.push_back({host, port});
    }
}

}  // namespace

StatusServiceImpl::StatusServiceImpl(const std::string& config_path) {
    boost::property_tree::ptree config;
    boost::property_tree::read_ini(config_path, config);
    AddServer(config, "ChatServer1", chat_servers_);
    AddServer(config, "ChatServer2", chat_servers_);
    if (chat_servers_.empty()) {
        throw std::runtime_error("StatusServer has no configured ChatServer endpoint");
    }
}

grpc::Status StatusServiceImpl::GetChatServer(
    grpc::ServerContext* /*context*/,
    const message::GetChatServerReq* /*request*/,
    message::GetChatServerRsp* reply) {
    const auto index = next_server_.fetch_add(1) % chat_servers_.size();
    const auto& server = chat_servers_[index];
    reply->set_error(0);
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_token(GenerateToken());
    return grpc::Status::OK;
}

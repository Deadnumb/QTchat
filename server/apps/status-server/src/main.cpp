#include "StatusServiceImpl.h"

#include <cstdlib>
#include <grpcpp/grpcpp.h>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char** argv) {
    const std::string config_path = argc > 1 ? argv[1] : "config.ini";

    try {
        StatusServiceImpl service(config_path);
        boost::property_tree::ptree config;
        boost::property_tree::read_ini(config_path, config);
        const auto host = config.get<std::string>("StatusServer.Host", "127.0.0.1");
        const auto port = config.get<std::string>("StatusServer.Port", "50052");
        const auto address = host + ":" + port;

        grpc::ServerBuilder builder;
        builder.AddListeningPort(address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        auto server = builder.BuildAndStart();
        if (!server) {
            std::cerr << "failed to start StatusServer on " << address << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "StatusServer listening on " << address << std::endl;
        server->Wait();
    } catch (const std::exception& error) {
        std::cerr << "StatusServer error: " << error.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

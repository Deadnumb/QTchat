#include "CServer.h"
#include <ConfigManager.h>
#include <iostream>
#include "global.h"
#include <cstdlib>
#include "RedisManager.h"


int main(int argc, char* argv[])
{
    auto &gCfgManager = ConfigManager::get_instance();
    std::string gate_port_str = gCfgManager["GateServer"]["Port"];
    unsigned short gate_port = atoi(gate_port_str.c_str());
    try
    {
        unsigned short port = gate_port;
        net::io_context ioc{ 1 };
        // 接受信号用来优雅退出
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {

            if (error) {
                return;
            }
            ioc.stop();
        });

        std::make_shared<CServer>(ioc, port)->Start();
        std::cout << "Port listen to: " << port << std::endl;
        ioc.run();

    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

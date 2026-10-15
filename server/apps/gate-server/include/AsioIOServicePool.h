#ifndef ASIOIOSERVICEPOOL_H
#define ASIOIOSERVICEPOOL_H

#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"

class AsioIOServicePool:public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;
public:
    ~AsioIOServicePool();
    AsioIOServicePool& operator = (const AsioIOServicePool&) = delete;
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    using IOService = boost::asio::io_context;
    using Work = boost::asio::executor_work_guard<IOService::executor_type>;
    using WorkPtr = std::unique_ptr<Work>;
    boost::asio::io_context& GetIOService();
    void Stop();

private:
    AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency() * 2);
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    std::size_t _nextIOService = 0;
};



#endif /* ASIOIOSERVICEPOOL_H */

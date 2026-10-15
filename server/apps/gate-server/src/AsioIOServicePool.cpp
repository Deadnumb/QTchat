#include "AsioIOServicePool.h"


AsioIOServicePool::AsioIOServicePool(std::size_t size):_ioServices(size),
 _works(size), _nextIOService(0)
{
    // 初始化_works数组里的每个成员，这些成员都是Work类型，参数为_ioServices的executor
    for(std::size_t i = 0; i < size;i++){
        _works[i] = std::unique_ptr<Work>(new Work(_ioServices[i].get_executor()));
    }
    // 遍历_ioServices数组，对每一个_ioService启动一个新线程管理
    // _threads可能接受的参数为函数对象
    for(std::size_t i = 0; i < _ioServices.size(); i++ ){
        _threads.emplace_back([this, i](){
            _ioServices[i].run();
        });
    }

}



AsioIOServicePool::~AsioIOServicePool()
{
    Stop();
    std::cout << "AsioIOServicePool destructed" << std::endl;
}

void AsioIOServicePool::Stop()
{
    for(auto &work: _works){
        if(work){
            work.reset();
        }
    }
    for(auto &io: _ioServices){
        io.stop();
    }
    for(auto &thread: _threads){
        if(thread.joinable()){
            thread.join();
        }
    }
}

boost::asio::io_context& AsioIOServicePool::GetIOService()
{
    auto &service = _ioServices[_nextIOService];
    _nextIOService = (_nextIOService + 1) % _ioServices.size();
    return service;
}

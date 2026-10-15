#include "CServer.h"
#include "HttpConnection.h"
#include <memory>
#include <iostream>
#include "AsioIOServicePool.h"
CServer::CServer(boost::asio::io_context &ioc, unsigned short &port):_ioc(ioc),
_acceptor(ioc, tcp::endpoint(tcp::v4(), port))
{

}

void CServer::Start()
{
    auto self = shared_from_this();
    // 从连接池获取io_context
    auto &io_context = AsioIOServicePool::get_instance()->GetIOService();
    // 再初始化新的连接
    std::shared_ptr<HttpConnection> new_connection = std::make_shared<HttpConnection>(io_context);
    _acceptor.async_accept(new_connection->GetSocket(), [self, new_connection](beast::error_code ec){
        try
        {
            // 出错就放弃连接，继续监听其它连接
            if(ec){
                self->Start();
                return;
            }
            //HttpConnection开始处理连接
            new_connection->Start();
            //移交会话后继续监听
            self->Start();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

    });
}

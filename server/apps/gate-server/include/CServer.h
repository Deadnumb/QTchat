#ifndef CServer_H
#define CServer_H

#include "global.h"

class CServer:public std::enable_shared_from_this<CServer>
{
public:
    friend class HttpConnection;
    CServer(boost::asio::io_context& ioc, unsigned short &port);
    void Start();

private:
    net::io_context& _ioc;
    tcp::acceptor _acceptor;


};


#endif

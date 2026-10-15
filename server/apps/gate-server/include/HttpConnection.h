#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "global.h"
#include "utils.h"
class LogicSystem;

class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
    //单例业务函数
    friend class LogicSystem;
public:
    //构造函数传入上下文
    HttpConnection(boost::asio::io_context & ioc);

    // 获取套接字
    tcp::socket& GetSocket();
    //启动
    void Start();
private:
    /*
    async_read(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    ReadHandler&& handler)
    第一个参数为异步可读的数据流，大家可以理解为socket.

    第二个参数为一个buffer，用来存储接受的数据，因为http可接受文本，图像，音频等多种资源文件，所以是Dynamic动态类型的buffer。

    第三个参数是请求参数，我们一般也要传递能接受多种资源类型的请求参数。

    第四个参数为回调函数，接受成功或者失败，都会触发回调函数，我们用lambda表达式就可以了。
     */

    //超时检测
    void CheckDeadline();
    // 回应
    // WriteResponse里的回调函数触发之后会取消定时器的回调，避免定时器把连接关闭
    // 如果WriteRespones里的回调没有触发，那么定时器把会话关闭
    // 可能触发timelater



    void WriteResponse();
    //处理请求
    void HandleReq();

    //解析url参数
    void PreParseGetParam();
    //必要的成员
    tcp::socket _socket;
    beast::flat_buffer _buffer{ 8162 };

    http::request<http::dynamic_body> _request;
    http::response<http::dynamic_body> _response;

    net::steady_timer deadline_{
        _socket.get_executor(), std::chrono::seconds(60)
    };

    std::string _get_url;
    std::unordered_map<std::string, std::string> _get_params;
};



#endif /* HTTPCONNECTION_H */

#include "HttpConnection.h"
#include "LogicSystem.h"
#include "utils.h"

HttpConnection::HttpConnection(boost::asio::io_context & ioc):_socket(ioc)
{

}

tcp::socket& HttpConnection::GetSocket()
{
    return _socket;
}


void HttpConnection::Start()
{
    auto self = shared_from_this();

        // std::cout << "[DEBUG] New connection accepted, socket fd: "
        //       << _socket.native_handle()
        //       << ", remote endpoint: "
        //       << _socket.remote_endpoint().address().to_string()
        //       << ":" << _socket.remote_endpoint().port()
        //       << std::endl;
    http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, size_t bytes_transsfered){
        try
        {
            if(ec){
                std::cout << "http read err is" << ec.what() << std::endl;
                return;
            }

            boost::ignore_unused(bytes_transsfered);
            self->HandleReq();
            self->CheckDeadline();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

    });
}

void HttpConnection::CheckDeadline()
{
    auto self = shared_from_this();

    deadline_.async_wait(
        [self](beast::error_code ec)
        {
            if (!ec)
            {
                // Close socket to cancel any outstanding operation.
                self->_socket.close(ec);
            }
        });
}


void HttpConnection::HandleReq()
{
    _request.version(_request.version());
    // _request.keep_alive(false);

    // 设置HTTP头
    if(_request.method() == http::verb::get){
        // 提取参数
        PreParseGetParam();
        bool success = LogicSystem::get_instance()->HandleGet(_get_url, shared_from_this());

        if(!success){
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }
        _response.result(http::status::ok);
        _response.set(http::field::content_type, "GateServer");
        WriteResponse();
        return;
    }
    else if(_request.method() == http::verb::post){
        bool success = LogicSystem::get_instance()->HandlePost(_request.target(), shared_from_this());

        if(!success){
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }
        _response.result(http::status::ok);
        _response.set(http::field::content_type, "GateServer");
        WriteResponse();
        return;
    }
}

void HttpConnection::WriteResponse()
{
    auto self = shared_from_this();
    _response.content_length(_response.body().size());
    // http::async_write(
    //     _socket,
    //     _response,
    //     [self](beast::error_code ec, std::size_t)
    //     {
    //         self->_socket.shutdown(tcp::socket::shutdown_send, ec);
    //         self->deadline_.cancel();
    //     });
    bool keep_alive = self->_request.keep_alive();
    _response.keep_alive(keep_alive);
    http::async_write(
        _socket,
        _response,
        [self, keep_alive](beast::error_code ec, std::size_t)
        {
            if (ec) {
                std::cerr << "write error: " << ec.message() << std::endl;
                self->_socket.close(ec);
                return;
            }

            // 如果要求保持连接，则继续读取下一个请求
            if (keep_alive) {
                // 清空响应，准备下一次读取
                self->_response = {};
                self->_request = {};
                self->_buffer.consume(self->_buffer.size());
                // 继续读取下一个请求
                self->Start();
            } else {
                // 客户端要求关闭，才关闭连接
                self->_socket.shutdown(tcp::socket::shutdown_send, ec);
                self->deadline_.cancel();
            }
        });
}

void HttpConnection::PreParseGetParam()
{
    // 提取 URI
    auto uri = _request.target();
    // 查找查询字符串的开始位置（即 '?' 的位置）
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos) {
        _get_url = uri;
        return;
    }

    _get_url = uri.substr(0, query_pos);
    std::string query_string = uri.substr(query_pos + 1);
    std::string key;
    std::string value;
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码
            value = UrlDecode(pair.substr(eq_pos + 1));
            _get_params[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    // 处理最后一个参数对（如果没有 & 分隔符）
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(query_string.substr(0, eq_pos));
            value = UrlDecode(query_string.substr(eq_pos + 1));
            _get_params[key] = value;
        }
    }
}

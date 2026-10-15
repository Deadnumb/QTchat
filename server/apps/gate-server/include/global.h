#ifndef GLOBAL_H
#define GLOBAL_H

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <hiredis/hiredis.h>
#include <cassert>
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCode{
    Success = 0,
    Error_json = 1001,
    RPCFailed = 1002,
    PasswdErr = 1003,
    UserExist = 1004,
    VarifyCodeErr = 1005,
    VarifyExpired = 1006,
    EmailExist = 1007,
    DatabaseError = 1008,
    LoginFailed = 1009,
    RedisError = 1010,
    ConfigError = 1011
};


#endif /* GLOBAL_H */

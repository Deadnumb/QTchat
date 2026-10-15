#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "StatusGrpcClient.h"

#include <cstddef>
#include <chrono>

static const std::string CODEPREFIX = "verify_code_";
static const std::string TOKENPREFIX = "user_token_";
static const auto TOKENTTL = std::chrono::seconds(3600);

void LogicSystem::RegGet(std::string url, HttpHandler handler)
{
    _get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler)
{
    _post_handlers.insert(make_pair(url, handler));
}

// 对Get和Post请求的处理函数，注册到LogicSystem中
LogicSystem::LogicSystem()
{
    // RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection){
    //     beast::ostream(connection->_response.body()) << "receive get_test req.";
    // });
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        beast::ostream(connection->_response.body()) << "receive get_test req " << std::endl;
        int i = 0;
        for (auto& elem : connection->_get_params) {
            i++;
            beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
            beast::ostream(connection->_response.body()) << ", " <<  " value is " << elem.second << std::endl;
        }
    });

    RegPost("/get_verifycode", [](std::shared_ptr<HttpConnection> connection){
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "received HTTP request body" << std::endl;
        connection->_request.set(http::field::content_type, "plain/text");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;

        bool parse_success = reader.parse(body_str, src_root);
        if(!parse_success){
            std::cout << "Failed to parse Json Data! " << std::endl;
            root["error"] = ErrorCode::Error_json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }


        if(!src_root.isMember("email")){
            std::cout << "Failed to parse Json Data! " << std::endl;
            root["error"] = ErrorCode::Error_json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        auto email = src_root["email"].asString();

        const auto email_check = MySQLManager::get_instance()->CheckEmailExists(email);
        if (email_check == EmailCheckResult::Exists) {
            std::cout << " email already registered, skip verification code" << std::endl;
            root["error"] = ErrorCode::EmailExist;
            root["email"] = src_root["email"];
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }
        if (email_check == EmailCheckResult::QueryFailed) {
            std::cout << " email existence check failed" << std::endl;
            root["error"] = ErrorCode::DatabaseError;
            root["email"] = src_root["email"];
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        GetVerifyRsp rsp = VerifyGrpcClient::get_instance()->GetVerifyCode(email);

        std::cout << "email is " << email << std::endl;
        // 错误码按域管理：VerifyServer 的内部 Go 码(0/1/2)不出边界，
        // 在这里统一翻译成客户端域的 ErrorCode 再返回
        root["error"] = (rsp.error() == ErrorCode::Success) ? ErrorCode::Success : ErrorCode::RPCFailed;
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString();
        // 这个ostream不是很会用
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
    });


    // 登录
    RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
        const auto body_str =
            boost::beast::buffers_to_string(
                connection->_request.body().data());

        connection->_response.set(
            http::field::content_type, "application/json");

        Json::Value response;
        Json::Value request;
        Json::Reader reader;

        const auto write_error = [&](int error) {
            response["error"] = error;
            beast::ostream(connection->_response.body())
                << response.toStyledString();
        };

        if (!reader.parse(body_str, request)
            || !request.isMember("user")
            || !request.isMember("passwd")) {
            write_error(ErrorCode::Error_json);
            return;
        }

        const auto name = request["user"].asString();
        const auto pwd = request["passwd"].asString();

        int uid = 0;
        const auto login_result =
            MySQLManager::get_instance()->CheckLogin(name, pwd, uid);

        if (login_result == LoginCheckResult::InvalidCredentials) {
            write_error(ErrorCode::LoginFailed);
            return;
        }
        if (login_result == LoginCheckResult::QueryFailed) {
            write_error(ErrorCode::DatabaseError);
            return;
        }

        const auto status_reply =
            StatusGrpcClient::get_instance()->GetChatServer(uid);
        if (status_reply.error() != ErrorCode::Success
            || status_reply.host().empty()
            || status_reply.port().empty()
            || status_reply.token().empty()) {
            write_error(ErrorCode::RPCFailed);
            return;
        }

        int chat_port = 0;
        try {
            chat_port = std::stoi(status_reply.port());
        } catch (const std::exception&) {
            write_error(ErrorCode::RPCFailed);
            return;
        }

        if (chat_port <= 0 || chat_port > 65535) {
            write_error(ErrorCode::RPCFailed);
            return;
        }

        const auto token = status_reply.token();
        try {
            const bool stored = (*RedisManager::get_instance())->set(
                TOKENPREFIX + std::to_string(uid),
                token,
                TOKENTTL);
            if (!stored) {
                write_error(ErrorCode::RedisError);
                return;
            }
        } catch (const std::exception& e) {
            std::cerr << "Redis token write failed: "
                      << e.what() << std::endl;
            write_error(ErrorCode::RedisError);
            return;
        }

        response["error"] = ErrorCode::Success;
        response["uid"] = uid;
        response["host"] = status_reply.host();
        response["port"] = chat_port;
        response["token"] = token;

        beast::ostream(connection->_response.body())
            << response.toStyledString();
    });
    RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
    auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
    std::cout << "received HTTP request body" << std::endl;
    connection->_response.set(http::field::content_type, "text/json");
    Json::Value root;
    Json::Reader reader;
    Json::Value src_root;
    bool parse_success = reader.parse(body_str, src_root);
    if (!parse_success) {
        std::cout << "Failed to parse JSON data!" << std::endl;
        root["error"] = ErrorCode::Error_json;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
    }

    auto email = src_root["email"].asString();
    auto name = src_root["user"].asString();
    auto pwd = src_root["passwd"].asString();
    auto confirm = src_root["confirm"].asString();

    if (pwd != confirm) {
        std::cout << "password err " << std::endl;
        root["error"] = ErrorCode::PasswdErr;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
    }

    //先查找redis中email对应的验证码是否合理
    std::string  varify_code;
    auto varify_code_opt = (*RedisManager::get_instance())->get(CODEPREFIX + src_root["email"].asString());
    bool b_get_varify = varify_code_opt.has_value();
    if (b_get_varify) {
        varify_code = *varify_code_opt;
    }
    if (!b_get_varify) {
        std::cout << " get varify code expired" << std::endl;
        root["error"] = ErrorCode::VarifyExpired;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
    }

    if (varify_code != src_root["varifycode"].asString()) {
        std::cout << " varify code error" << std::endl;
        root["error"] = ErrorCode::VarifyCodeErr;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
    }

    //查找数据库判断用户是否存在
    int uid = MySQLManager::get_instance()->RegUser(name, email, pwd);
    if (uid == 0) {
        std::cout << " user or email exist" << std::endl;
        root["error"] = ErrorCode::UserExist;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
    }
    if (uid == -1) {
        std::cout << " database registration failed" << std::endl;
        root["error"] = ErrorCode::DatabaseError;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
    }

    // 注册成功，返回用户信息
    root["error"] = 0;
    root["uid"] = uid;
    root["email"] = email;
    root ["user"]= name;

    std::string jsonstr = root.toStyledString();
    beast::ostream(connection->_response.body()) << jsonstr;
    return true;
    });
}


bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con)
{
    if(_get_handlers.find(path) == _get_handlers.end()){
        return false;
    }
    _get_handlers[path](con);
    return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con)
{
    if(_post_handlers.find(path) == _post_handlers.end()){
        return  false;
    }
    _post_handlers[path](con);
    return true;
}




LogicSystem::~LogicSystem()
{

}

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <functional>
#include "QStyle"
#include <QRegularExpression>
#include <memory>
#include <mutex>
#include <iostream>
#include <QJsonObject>
#include <QDir>
#include <QSettings>
/*
 * @brief repolish
 */
extern std::function<void(QWidget*)> repolish;

enum class ReqId
{
    ID_GET_VERIFY_CODE = 1001,
    ID_REG_USER = 1002,
    ID_LOGIN_USER = 1003,
};

enum class Modules
{
    REGISTERMOD = 0,
    LOGINMOD = 1
};


enum ErrorCodes
{
    SUCCESS = 0,
    ERROR_JSON = 1,
    ERROR_NETWORK = 2
};

// 与 GateServer 约定的业务错误码（对应后端 enum ErrorCode）。
// 注意：上面的 ErrorCodes 是客户端自身的错误分类（网络/JSON解析），
// 这里的 GateErrorCode 是服务器下发的业务错误码，两者来源不同，不要混用
enum GateErrorCode
{
    GATE_SUCCESS        = 0,
    GATE_ERROR_JSON     = 1001,
    GATE_RPC_FAILED     = 1002,
    GATE_PASSWD_ERR     = 1003,
    GATE_USER_EXIST     = 1004,
    GATE_VARIFY_ERR     = 1005,
    GATE_VARIFY_EXPIRED = 1006,
    GATE_EMAIL_EXIST    = 1007,
    GATE_DATABASE_ERR   = 1008,
    GATE_LOGIN_FAILED   = 1009,
    GATE_REDIS_ERR      = 1010,
    GATE_CONFIG_ERR     = 1011
};

extern QString gate_url_prefix;


#endif // GLOBAL_H

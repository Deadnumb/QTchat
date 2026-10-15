---
status: ready
estimated_hours: 3
blocked_by: 01-registration-baseline
expanded_at: 2026-08-28
based_on: 2026-08-28 当前 WSL GateServer、VerifyServer 与 D:/QTChat/QTChat 源码快照
reference: https://www.yuque.com/lianlianfengchen-cvvh2/dz8xhn/ig1no1iid1y1hmet
---

# 第 02 课：完成 HTTP 登录与 ChatServer 交接

## 0. 先读这段：本课到底要完成什么

第 01 课已经关闭：验证码注册可以从 Qt 走到 GateServer、VerifyServer、Redis 和 MySQL，最终在 user 表中产生一个 User。

第 02 课只处理登录的 HTTP 阶段。它要把“凭证正确的 User”交接给后续的 TCP ChatServer，但本课不实现 TCP ChatServer，也不在登录成功前连接 TCP。

本文是基于当前真实工作区写出的可执行教学稿。当前代码中：

- C++ 目录名是 ChatServer，但它现在实际承担 GateServer 职责；
- GateServer 当前只有 /get_verifycode 和 /user_register，没有 /user_login；
- MySQLDAO 当前有 RegUser() 和 CheckEmailExists()，还没有登录查询；
- Qt 的 LoginDialog 只有界面和注册按钮切换，没有登录请求处理；
- TCP ChatServer 尚未建立。

因此，下面的代码块是学习者本课要新增的代码，不是当前已经存在的实现。完成后把本文件保持为 ready，等学习者实际验收后再将本课改为 closed。

本课沿用 CONTEXT.md 中记录的教学博客作为参考，但课程范围、错误码和最终行为以 ROADMAP、ADR 0002 以及当前源码为准。

## 0.1 先确定架构，再看代码

本课采用 ADR 0002 的单 ChatServer 简化架构，不引入 StatusServer，也不要求学习者先实现 RPC。这里的“交接”只是 GateServer 把登录后的连接信息交给 Qt，下一课再由 Qt 建立 TCP 连接。

```text
Qt LoginDialog
    │ HTTP：user + passwd
    ▼
GateServer LogicSystem
    ├─ MySQL：校验凭证，产生 uid
    ├─ Redis：保存 uid 对应的短时 token
    └─ Config：读取 ChatServer host/port
    ▼
HTTP：LoginResult{uid, host, port, token}
    │
    ▼
Qt LoginSession
```

本课所有层都遵守同一个输入输出契约：客户端只提供 `user` 和 `passwd`；DAO/Manager 返回一个完整的登录结果；HTTP 层只负责把结果转换成业务错误或 JSON。`uid` 是查询结果，不是请求参数，也不通过 `int&`、裸指针或其他隐式写回方式传递。

## 1. 完成后你会看到什么

Qt 登录窗口输入用户名和密码后，会向 GateServer 发送：

~~~json
{
  "user": "Deadnumb",
  "passwd": "注册时设置的密码"
}
~~~

正确凭证会收到类似下面的交接数据：

~~~json
{
  "error": 0,
  "uid": 5,
  "host": "127.0.0.1",
  "port": 9000,
  "token": "<opaque-token>"
}
~~~

实际 token 每次都不同，不要把它固定写进代码、日志或截图。

登录成功后，本课只把 uid、host、port、token 保存在 Qt 的登录会话对象并发出 loginSuccess 信号。下一课才使用这些数据建立 TCP 连接。

错误凭证返回 1009，Redis 写 token 失败返回 1010，ChatServer 地址配置缺失或非法返回 1011。客户端要把这些 GateServer 业务错误转换成用户能理解的提示。

## 2. 动手前需要理解的概念

### 2.1 登录不是注册

注册是“验证码通过后创建 User”，登录是“用已有 User 的凭证换取一次连接交接信息”。登录绝不能调用 RegUser()，因为 RegUser() 会产生注册副作用。

登录查询只需要：

~~~sql
SELECT uid
FROM `user`
WHERE name = ? AND pwd = ?
LIMIT 1;
~~~

这里的问号必须通过 PreparedStatement 绑定，不能拼接字符串。

本课不使用“返回状态码、再通过 `int& uid` 偷偷写回结果”的接口。那种写法虽然是合法 C++，但把一个完整的查询结果拆成了两个出口，调用者还必须记住“只有成功时 uid 才有效”。本课使用一个结果对象，把状态和 uid 放在同一个返回值里。

当前 demo 的 pwd 仍然是数据库中的现有明文密码字段。本课不把它包装成安全方案；生产系统应使用密码哈希、TLS 和更严格的凭证策略。

### 2.2 token 是 HTTP 到 TCP 的交接凭证

GateServer 不把数据库密码交给 ChatServer。登录成功后生成一个短时效 token，并按 User 的 uid 写入 Redis：

~~~text
key:   user_token_<uid>
value: <随机 token>
TTL:   3600 秒
~~~

未来的 TCP ChatServer 收到 uid 和 token 后，会用同一个 Redis key 校验它。本课只建立这个契约，不实现校验端。

### 2.3 TTL 是生命周期边界

Redis 中的 token 不是永久登录状态。TTL 过期后，token 会自动失效，Redis 返回 TTL=-2 表示 key 不存在。登录成功不等于永远在线，后续课程还会处理 TCP Session 生命周期。

### 2.4 错误码要在服务边界转换

Qt 自己的 ErrorCodes 只表示网络错误和 JSON 解析错误；GateErrorCode 表示 GateServer 的业务结果。二者不能混用。

本课新增的 GateServer 错误码：

| 错误码 | 含义 | 客户端提示 |
| ---: | --- | --- |
| 1009 | 用户名或密码错误 | 用户名或密码错误 |
| 1010 | Redis token 写入失败 | 登录服务暂时不可用，请稍后重试 |
| 1011 | ChatServer 地址配置错误 | 登录服务配置错误 |

### 2.5 当前没有 StatusServer

ADR 0002 已决定：第一个 demo 只使用一个配置好的 ChatServer。GateServer 直接从配置返回 host 和 port，不增加 StatusServer，也不让 Qt 自己猜服务地址。

本课的 port=9000 只是为下一课预留的固定本地端口。当前 TCP ChatServer 尚未监听 9000，所以本课看到 9000 没有监听是正常的。

### 2.6 不要使用当前 xorString 处理密码

当前 Qt 的 global.cpp 中有一个名为 xorString 的旧工具。它不是加密：

- XOR 使用的是固定的短密钥，知道规则就能还原；
- 当前 lambda 在循环内部提前 return，只会处理第一个字符；
- 对用户名或密码调用它会改变用户实际输入，导致登录失败；
- 本课的登录请求必须直接使用 QLineEdit 的文本，不调用 xorString。

这不是本课要修复的功能。第 10 课统一清理日志、明文密码和误导性的安全命名。

## 3. 当前调用链与目标调用链

### 3.1 当前调用链

~~~text
Qt LoginDialog
  └─ 点击“登录”
       └─ 当前没有登录 slot
            └─ 不会发送 HTTP

GateServer
  └─ 当前没有 /user_login

MySQL
  └─ user 表存在，但没有登录查询入口
~~~

### 3.2 本课目标调用链

~~~text
Qt LoginDialog
  └─ on_login_btn_clicked()
       └─ HttpManager::PostHttpReq()
            └─ POST /user_login
                 └─ GateServer LogicSystem
                      ├─ MySQLDAO::CheckLogin()
                      │    ├─ Success → LoginQueryResult{status, uid}
                      │    ├─ InvalidCredentials → 1009
                      │    └─ QueryFailed → 1008
                      ├─ 生成随机开发 token
                      ├─ Redis SET user_token_<uid> EX 3600
                      │    └─ 失败 → 1010
                      ├─ 读取 [ChatServer] Host/Port
                      │    └─ 缺失或非法 → 1011
                      └─ 返回 uid、host、port、token
                           └─ Qt 保存 LoginSession
~~~

注意：这个调用链到 HTTP 回包为止。它不包含 QTcpSocket::connectToHost()，也不应在本课加入这一步。

## 4. 开工前检查

以下检查是只读的。先确认你是在当前真实工作区上工作，不要拿旧博客代码覆盖现有文件。

### 4.1 检查工具、容器和端口

~~~bash
zsh -lic 'go version'
zsh -lic 'command -v docker && docker ps -a --format "{{.Names}} {{.Status}}"'
ss -ltnp | grep -E ':(8080|50051)( |$)' || true
~~~

预期：

- VerifyServer 依赖的 Go 可执行；
- 容器名能看到 chatmysql 和 chatredis；
- GateServer 监听 8080，VerifyServer 监听 50051；
- 9000 暂时可以没有监听，因为 TCP ChatServer 尚未实现。

### 4.2 确认数据库中已有可登录 User

不要把真实密码写进教学材料。使用本机私密环境变量：

~~~bash
export CHAT_MYSQL_PASSWORD='<本机 MySQL 密码>'
docker exec -e MYSQL_PWD="$CHAT_MYSQL_PASSWORD" chatmysql mysql -uroot -N -B chatdb -e 'SELECT uid, name, email FROM user ORDER BY uid DESC LIMIT 5;'
~~~

使用第 01 课刚注册的 User 进行登录。若不知道密码，不要从日志中找；重新注册一个本地演示 User，或使用你自己记得的测试账号。

### 4.3 检查当前源码入口

~~~bash
cd /home/deadnumb/C++ChatProject/apps/gate-server
grep -RInE 'RegPost|user_register|MySQLDAO|RedisManager' include src
~~~

~~~powershell
rg -n 'class LoginDialog|PostHttpReq|sig_reg_mod_finish|on_login_btn_clicked' D:/QTChat/QTChat
~~~

当前搜索结果应能看到注册链路和空的 LoginDialog，而不是已经完成的登录接口。

## 5. 按顺序实现

本课故意保持纵向小步：每一步完成一个边界，再继续下一步。每次改动后先编译对应项目。

### 5.1 先在配置中声明唯一 ChatServer

编辑 GateServer 工作目录中的 apps/gate-server/config.ini，在现有 VerifyServer 段后添加：

~~~ini
[ChatServer]
Host = 127.0.0.1
Port = 9000
~~~

这里的配置只是在登录契约中声明未来 TCP ChatServer 的地址。当前没有 TCP 服务，不要为了让本课“看起来完整”临时写一个假 TCP 服务。

ConfigManager 按当前工作目录读取 config.ini，并且键名大小写敏感。GateServer 必须从 out/build/gate-server-debug/bin 启动，确保它能读到 out/build/gate-server-debug/bin/config.ini。

### 5.2 增加 GateServer 登录错误码

在 apps/gate-server/include/global.h 的 ErrorCode 末尾增加：

~~~cpp
enum ErrorCode {
    // 已有 1000~1008 ...
    LoginFailed = 1009,
    RedisError = 1010,
    ConfigError = 1011
};
~~~

不要重排或复用前面已经用于注册的错误码。客户端会按数字契约映射它们。

### 5.3 给 MySQLDAO 增加纯登录查询

在 apps/gate-server/include/MySQLDao.h 中，放在 EmailCheckResult 附近：

~~~cpp
enum class LoginCheckResult {
    Success,
    InvalidCredentials,
    QueryFailed
};

struct LoginQueryResult {
    LoginCheckResult status = LoginCheckResult::QueryFailed;
    int uid = 0;
};
~~~

在 MySQLDAO 公有方法中增加：

~~~cpp
LoginQueryResult CheckLogin(const std::string& name,
                            const std::string& pwd);
~~~

在 apps/gate-server/src/MySQLDao.cpp 中实现：

~~~cpp
LoginQueryResult MySQLDAO::CheckLogin(const std::string& name,
                                      const std::string& pwd) {
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return {LoginCheckResult::QueryFailed, 0};
    }

    try {
        std::unique_ptr<sql::PreparedStatement> stmt(
            con->con_->prepareStatement(
                "SELECT uid FROM `user` "
                "WHERE `name` = ? AND `pwd` = ? LIMIT 1"));
        stmt->setString(1, name);
        stmt->setString(2, pwd);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if (!res->next()) {
            pool_->returnConnection(std::move(con));
            return {LoginCheckResult::InvalidCredentials, 0};
        }

        const int uid = res->getInt("uid");
        pool_->returnConnection(std::move(con));
        return {LoginCheckResult::Success, uid};
    } catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException while checking login: "
                  << e.what()
                  << " (MySQL error code: " << e.getErrorCode()
                  << ", SQLState: " << e.getSQLState() << ")"
                  << std::endl;
        return {LoginCheckResult::QueryFailed, 0};
    }
}
~~~

关键点：

- `LoginQueryResult` 同时表达查询状态和成功时的 uid；
- 没有匹配行是 InvalidCredentials，不是数据库异常；
- 每条路径都必须把连接归还连接池；
- PreparedStatement 负责绑定输入，不能把 name 和 pwd 拼到 SQL 字符串中；
- 不要在这里调用 RegUser()。

### 5.4 通过 MySQLManager 暴露同一能力

在 apps/gate-server/include/MySQLManager.h 增加：

~~~cpp
LoginQueryResult CheckLogin(const std::string& name,
                            const std::string& pwd);
~~~

在 apps/gate-server/src/MySQLManager.cpp 增加：

~~~cpp
LoginQueryResult MySQLManager::CheckLogin(const std::string& name,
                                          const std::string& pwd) {
    return dao_.CheckLogin(name, pwd);
}
~~~

LogicSystem 只依赖 Manager，不直接接触连接池。这样 GateServer 的 HTTP 层不会和 MySQL 连接细节耦合。

### 5.5 在 LogicSystem 中生成 token 并注册 /user_login

在 apps/gate-server/src/LogicSystem.cpp 顶部增加：

~~~cpp
#include <chrono>
#include <random>
#include <string>
~~~

在文件顶部的 CODEPREFIX 附近增加一个匿名命名空间：

~~~cpp
namespace {

constexpr char TOKEN_ALPHABET[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr std::size_t TOKEN_LENGTH = 32;
const std::string TOKEN_PREFIX = "user_token_";
const auto TOKEN_TTL = std::chrono::seconds(3600);

std::string GenerateToken() {
    static thread_local std::mt19937_64 engine(std::random_device{}());
    std::uniform_int_distribution<std::size_t> dist(
        0, sizeof(TOKEN_ALPHABET) - 2);

    std::string token;
    token.reserve(TOKEN_LENGTH);
    for (std::size_t i = 0; i < TOKEN_LENGTH; ++i) {
        token.push_back(TOKEN_ALPHABET[dist(engine)]);
    }
    return token;
}

}  // namespace
~~~

这是本地教学 token 生成器，不是生产级密钥生成方案。它的重点是不要使用固定 token、时间戳拼接或当前 XOR 工具。

在 LogicSystem::LogicSystem() 中注册新的 POST handler。可以放在 /user_register handler 前面：

~~~cpp
RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
    auto body_str =
        boost::beast::buffers_to_string(
            connection->_request.body().data());
    connection->_response.set(
        http::field::content_type, "application/json");

    Json::Value root;
    Json::Value src_root;
    Json::Reader reader;

    auto write_error = [&](int error) {
        root["error"] = error;
        beast::ostream(connection->_response.body())
            << root.toStyledString();
    };

    if (!reader.parse(body_str, src_root)
        || !src_root.isMember("user")
        || !src_root.isMember("passwd")) {
        write_error(ErrorCode::Error_json);
        return;
    }

    const auto name = src_root["user"].asString();
    const auto pwd = src_root["passwd"].asString();

    const auto login_result =
        MySQLManager::get_instance()->CheckLogin(name, pwd);

    if (login_result.status == LoginCheckResult::InvalidCredentials) {
        write_error(ErrorCode::LoginFailed);
        return;
    }
    if (login_result.status == LoginCheckResult::QueryFailed) {
        write_error(ErrorCode::DatabaseError);
        return;
    }

    const int uid = login_result.uid;

    auto& cfg = ConfigManager::get_instance();
    const auto chat_host = cfg["ChatServer"]["Host"];
    const auto chat_port_text = cfg["ChatServer"]["Port"];

    int chat_port = 0;
    try {
        chat_port = std::stoi(chat_port_text);
    } catch (const std::exception&) {
        write_error(ErrorCode::ConfigError);
        return;
    }

    if (chat_host.empty() || chat_port <= 0 || chat_port > 65535) {
        write_error(ErrorCode::ConfigError);
        return;
    }

    const auto token = GenerateToken();
    try {
        (*RedisManager::get_instance())->set(
            TOKEN_PREFIX + std::to_string(uid),
            token,
            TOKEN_TTL);
    } catch (const std::exception& e) {
        std::cerr << "Redis token write failed: "
                  << e.what() << std::endl;
        write_error(ErrorCode::RedisError);
        return;
    }

    root["error"] = ErrorCode::Success;
    root["uid"] = uid;
    root["host"] = chat_host;
    root["port"] = chat_port;
    root["token"] = token;
    beast::ostream(connection->_response.body())
        << root.toStyledString();
});
~~~

这里的顺序有意安排为：

1. 解析 JSON；
2. 查询凭证；
3. 验证 ChatServer 配置；
4. 生成 token；
5. 写 Redis 并设置 TTL；
6. 返回完整交接数据。

凭证错误时不会生成 token；Redis 写入失败时不会返回成功；配置错误时不会让 Qt 得到一个无法使用的地址。

如果你的 redis-plus-plus 版本不接受 std::chrono::seconds 的 set 重载，可以改成先 set(key, token) 再调用 expire(key, 3600)，但必须检查两个操作的返回/异常，并在教学记录中说明这是两个 Redis 操作之间的窗口。优先使用带 TTL 的单次 set 重载。

### 5.6 给 Qt 增加登录请求 ID、模块和业务错误码

在 D:/QTChat/QTChat/global.h 中扩展现有枚举：

~~~cpp
enum class ReqId {
    ID_GET_VERIFY_CODE = 1001,
    ID_REG_USER = 1002,
    ID_LOGIN_USER = 1003
};

enum class Modules {
    REGISTERMOD = 0,
    LOGINMOD = 1
};
~~~

在现有 GateErrorCode 末尾增加：

~~~cpp
GATE_LOGIN_FAILED = 1009,
GATE_REDIS_ERR = 1010,
GATE_CONFIG_ERR = 1011
~~~

不要把 1009 等服务器业务码放进客户端 ErrorCodes。ERROR_NETWORK 仍然只代表 QNetworkReply 层面的失败。

### 5.7 给 HttpManager 增加登录回包路由

在 D:/QTChat/QTChat/httpmanager.h 的 signals 中增加：

~~~cpp
void sig_login_mod_finish(ReqId id, QString res, ErrorCodes err);
~~~

在 HttpManager::slot_http_finish() 中增加分支：

~~~cpp
if (mod == Modules::REGISTERMOD) {
    emit sig_reg_mod_finish(id, res, err);
} else if (mod == Modules::LOGINMOD) {
    emit sig_login_mod_finish(id, res, err);
}
~~~

PostHttpReq() 其余逻辑可以保持不变。它已经负责 Content-Type、异步 QNetworkReply、网络错误分类和 deleteLater()。

### 5.8 在 LoginDialog 中发送请求并保存交接数据

当前 LoginDialog 没有错误提示 label，因此本课用 QMessageBox 显示结果，不改 .ui 文件。

在 D:/QTChat/QTChat/logindialog.h 增加必要的 include、会话结构、slot、signal 和成员：

~~~cpp
#include "global.h"
#include <QMap>
#include <QString>
#include <functional>

struct LoginSession {
    int uid = 0;
    QString host;
    quint16 port = 0;
    QString token;

    bool isValid() const {
        return uid > 0
            && !host.isEmpty()
            && port > 0
            && !token.isEmpty();
    }
};
~~~

在 LoginDialog 类中增加：

~~~cpp
private slots:
    void on_login_btn_clicked();
    void slot_login_mod_finish(
        ReqId id, QString res, ErrorCodes err);

private:
    void initHttpHandlers();

    Ui::LoginDialog* ui;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    LoginSession _session;

signals:
    void switchRegister();
    void loginSuccess(
        int uid, QString host, quint16 port, QString token);
~~~

然后将 D:/QTChat/QTChat/logindialog.cpp 扩展为下面的逻辑。保留原来的构造、析构和注册按钮连接：

~~~cpp
#include "logindialog.h"
#include "ui_logindialog.h"
#include "global.h"
#include "httpmanager.h"

#include <QJsonDocument>
#include <QMessageBox>
#include <QDebug>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog) {
    ui->setupUi(this);
    ui->pass_edit->setEchoMode(QLineEdit::Password);

    connect(ui->register_btn, &QPushButton::clicked,
            this, &LoginDialog::switchRegister);
    connect(HttpManager::GetInstance().get(),
            &HttpManager::sig_login_mod_finish,
            this, &LoginDialog::slot_login_mod_finish);

    initHttpHandlers();
}

LoginDialog::~LoginDialog() {
    delete ui;
}

void LoginDialog::on_login_btn_clicked() {
    const auto user = ui->user_edit->text().trimmed();
    const auto passwd = ui->pass_edit->text();

    if (user.isEmpty() || passwd.isEmpty()) {
        QMessageBox::warning(this, tr("登录"),
                             tr("用户名和密码不能为空"));
        return;
    }

    QJsonObject json_obj;
    json_obj["user"] = user;
    // 不调用 xorString，直接发送用户输入。
    json_obj["passwd"] = passwd;

    HttpManager::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/user_login"),
        json_obj,
        ReqId::ID_LOGIN_USER,
        Modules::LOGINMOD);
}

void LoginDialog::slot_login_mod_finish(
    ReqId id, QString res, ErrorCodes err) {
    if (err != ErrorCodes::SUCCESS) {
        QMessageBox::warning(this, tr("登录"),
                             tr("网络请求失败"));
        return;
    }

    const auto doc =
        QJsonDocument::fromJson(res.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::warning(this, tr("登录"),
                             tr("服务器返回不是有效 JSON"));
        return;
    }

    const auto it = _handlers.find(id);
    if (it == _handlers.end()) {
        QMessageBox::warning(this, tr("登录"),
                             tr("找不到登录回包处理器"));
        return;
    }
    it.value()(doc.object());
}

void LoginDialog::initHttpHandlers() {
    _handlers.insert(
        ReqId::ID_LOGIN_USER,
        [this](const QJsonObject& json_obj) {
            const int error = json_obj["error"].toInt();
            if (error == GateErrorCode::GATE_SUCCESS) {
                _session.uid = json_obj["uid"].toInt();
                _session.host = json_obj["host"].toString();
                _session.port = static_cast<quint16>(
                    json_obj["port"].toInt());
                _session.token = json_obj["token"].toString();

                if (!_session.isValid()) {
                    QMessageBox::warning(
                        this, tr("登录"),
                        tr("登录交接数据不完整"));
                    return;
                }

                qDebug() << "Login handoff:"
                         << "uid=" << _session.uid
                         << "host=" << _session.host
                         << "port=" << _session.port;
                // 不打印 token。
                emit loginSuccess(
                    _session.uid,
                    _session.host,
                    _session.port,
                    _session.token);
                QMessageBox::information(
                    this, tr("登录"),
                    tr("登录成功，已取得 ChatServer 交接信息"));
                return;
            }

            if (error == GateErrorCode::GATE_LOGIN_FAILED) {
                QMessageBox::warning(
                    this, tr("登录"),
                    tr("用户名或密码错误"));
            } else if (error == GateErrorCode::GATE_REDIS_ERR) {
                QMessageBox::warning(
                    this, tr("登录"),
                    tr("登录服务暂时不可用，请稍后重试"));
            } else if (error == GateErrorCode::GATE_CONFIG_ERR) {
                QMessageBox::warning(
                    this, tr("登录"),
                    tr("登录服务配置错误"));
            } else {
                QMessageBox::warning(
                    this, tr("登录"),
                    tr("登录失败，错误码 %1").arg(error));
            }
        });
}
~~~

Qt 的自动连接规则会把 on_login_btn_clicked() 绑定到 objectName 为 login_btn 的按钮。若你的 Qt 版本或 UI 生成设置没有触发自动连接，可以在构造函数里显式补一条：

~~~cpp
connect(ui->login_btn, &QPushButton::clicked,
        this, &LoginDialog::on_login_btn_clicked);
~~~

不要同时保留两套连接，否则一次点击可能发送两次登录请求。

本课暂时不需要修改 MainWindow。loginSuccess 是给下一课或第四课使用的交接信号；本课先在 LoginDialog 中验证它收到并保存了四个字段。

## 6. 关键代码、所有权与生命周期

### 6.1 MySQL 连接的所有权

getConnection() 返回 std::unique_ptr<SQLConnection>，DAO 暂时拥有它。查询结束、返回结果前，必须通过 returnConnection(std::move(con)) 把所有权交回连接池。

不要在归还连接后继续访问 con。异常路径也要归还，否则登录请求多几次后连接池会逐渐耗尽。

当前连接池的线程安全和退出问题仍是后续加固项。本课的单次登录通过不等于连接池已经具备生产级稳定性。

### 6.2 token 的所有权

GenerateToken() 返回一个值类型 std::string。登录 handler 把它复制到 Redis 和 JSON 响应中；Redis 负责保存 TTL，Qt 收到响应后复制到 LoginSession::token。

不要保存指向局部 std::string 的裸指针，也不要把 token 写入日志。

### 6.3 Qt 网络回包的生命周期

HttpManager 是 Singleton 返回的 shared_ptr。PostHttpReq() 中的 shared_from_this() 会让异步 lambda 在请求完成前保持 HttpManager 存活。

QNetworkReply 必须使用 deleteLater()，不能在 finished 信号处理中直接 delete。登录回包先经过统一的 sig_http_finish，再按 Modules 分发到 sig_login_mod_finish，最后按 ReqId 分发到 _handlers。

### 6.4 为什么不能把 HTTP 成功当 TCP 成功

HTTP 成功只证明 GateServer 完成了凭证查询、token 写入和响应生成。当前没有 TCP ChatServer 监听 9000，所以登录成功后不会有 TCP 连接。下一课会单独建立 TCP 服务和帧协议。

### 6.5 课程回顾：为什么取消 `int& uid`

原稿中的 `int& uid` 是 C/C++ 常见的 out parameter：调用方先准备一个变量，函数执行时把查询到的 uid 写回去。它不是客户端传入 uid，客户端仍然只提交账号和密码。

但它不适合作为本课的推荐接口：返回值只表达状态，真正的数据藏在参数副作用里；失败时 uid 是否有效还要额外约定；而登录结果本来就是“状态 + uid”的整体。

因此本课统一使用结果对象：

~~~cpp
LoginQueryResult CheckLogin(const std::string& name,
                            const std::string& pwd);
~~~

调用方先判断 `status`，只有 `Success` 时才读取 `uid`。这样数据和状态会一起返回，也不会把 uid 误当成客户端输入。暂时不引入更复杂的 `std::expected`，避免把课程重点从登录链路带偏。

## 7. 构建和启动

### 7.1 启动依赖

终端一，启动 MySQL 和 Redis 容器：

~~~bash
docker start chatmysql chatredis
docker exec chatredis redis-cli -a "$CHAT_REDIS_PASSWORD" ping
~~~

预期为 PONG。密码通过环境变量提供，不要把真实密码写进命令截图。

终端二，启动 VerifyServer：

~~~bash
cd /home/deadnumb/C++ChatProject/apps/verify-server
zsh -lic 'go test ./...'
zsh -lic 'go build -o verify-server .'
./verify-server
~~~

预期监听 0.0.0.0:50051。

终端三，构建并启动 GateServer：

~~~bash
cd /home/deadnumb/C++ChatProject/apps/gate-server
cmake --build build --parallel 2
cd bin
./ChatServer
~~~

预期监听 0.0.0.0:8080。启动目录必须包含配置文件，且配置中有 [ChatServer] 段。

### 7.2 构建 Qt

在 Qt Creator 中用已有的 MinGW Kit 构建 D:/QTChat/QTChat。或者在已经配置好的构建目录执行：

~~~powershell
cmake --build D:/QTChat/QTChat/build --parallel 2
~~~

确认构建输出目录中存在 config.ini，且内容至少包括：

~~~ini
[GateServer]
host=127.0.0.1
port=8080
~~~

Qt 本课只需要 GateServer 地址；ChatServer 地址由登录响应下发。

## 8. 预期请求、响应、界面和 Redis

### 8.1 正确凭证

~~~bash
curl -sS -X POST -H 'Content-Type: application/json' -d '{"user":"你的用户名","passwd":"你的密码"}' http://127.0.0.1:8080/user_login
~~~

成功响应包含：

~~~json
{
  "error": 0,
  "uid": 5,
  "host": "127.0.0.1",
  "port": 9000,
  "token": "<opaque-token>"
}
~~~

不要把真实 token 粘贴到公开日志。可以只检查 key 是否存在、TTL 和 token 长度：

~~~bash
docker exec -e REDISCLI_AUTH="$CHAT_REDIS_PASSWORD" chatredis redis-cli EXISTS user_token_<uid>
docker exec -e REDISCLI_AUTH="$CHAT_REDIS_PASSWORD" chatredis redis-cli TTL user_token_<uid>
docker exec -e REDISCLI_AUTH="$CHAT_REDIS_PASSWORD" chatredis redis-cli STRLEN user_token_<uid>
~~~

预期依次是 1、接近 3600 的正数，以及大于 0 的长度。不要使用 GET 把 token 直接打印出来。

### 8.2 错误凭证

将密码改成错误值，预期：

~~~json
{"error":1009}
~~~

Qt 预期弹出“用户名或密码错误”，并且不会产生新的 user_token_<uid>。

### 8.3 Redis 故障

只在本地演示环境、确认可以恢复时做故障测试。停止 Redis 后再次登录，预期 GateServer 返回 1010，而不是 0。恢复容器后再继续验收。

### 8.4 配置错误

临时把 GateServer 工作目录中的 [ChatServer] 的 Port 改成非数字，重启 GateServer。登录应返回 1011。恢复为 9000 后重启。

### 8.5 Qt 界面

正确登录时：

1. LoginDialog 发出一次 POST；
2. 登录成功提示出现；
3. Qt 调试输出只包含 uid、host、port，不包含密码或 token；
4. 此时没有 TCP 连接；
5. loginSuccess 已携带四个交接字段。

## 9. 常见错误及排查顺序

| 现象 | 先查什么 | 结论或处理 |
| --- | --- | --- |
| POST 返回 404 | GateServer 是否为新构建并重启；是否注册 /user_login | 旧进程或忘记 RegPost |
| 正确密码仍返回 1009 | 用户名字段是否是 user；SQL 是否使用 name；是否连到正确 chatdb | JSON 字段和数据库列名是两套命名 |
| 返回 1008 | MySQL 容器、表结构、连接池日志 | 数据库查询失败，不要显示成密码错误 |
| 返回 1010 | Redis 是否 PONG；端口是否 6380；密码是否来自私密配置 | token 没有可靠写入，不能返回登录成功 |
| 返回 1011 | config.ini 工作目录、段名 ChatServer、键名 Host/Port 大小写 | 配置文件没被读取或值非法 |
| Qt 没有任何回包提示 | LOGINMOD 是否加入；HttpManager 是否 emit 登录信号；LoginDialog 是否 connect | 请求可能成功，但回包没有到正确 handler |
| 一次点击产生两条请求 | 检查 UI 自动连接和显式 connect 是否重复 | 只保留一种按钮连接方式 |
| Qt 报网络错误 | 检查输出目录 config.ini；PowerShell 执行 Test-NetConnection 127.0.0.1 -Port 8080 | 先排 HTTP，不要跳去排 TCP |
| Redis TTL 为 -2 | key 拼写、uid、Redis DB、登录是否返回成功 | key 不存在或已经过期 |
| token 每次都一样 | 是否误用了固定字符串、时间戳或 XOR | 重新检查 GenerateToken() |
| 登录后立刻连接 9000 失败 | 这是本课预期边界 | TCP ChatServer 在第 03 课建立，本课不要修成假连接 |
| MySQL 登录偶发失败 | 检查 MySQLPool 的线程和退出问题 | 这是已知后续加固项，不要用重试掩盖根因 |

建议的排查顺序固定为：

~~~text
Qt 网络层
  → GateServer HTTP 路由
    → MySQL 凭证查询
      → Redis token 写入
        → JSON 交接字段
          → Qt handler
~~~

不要一看到 TCP 连接失败就修改 HTTP 登录。

## 10. 端到端验收清单

### 后端

- [ ] apps/gate-server/config.ini 有 [ChatServer] Host/Port，GateServer 能读到。
- [ ] /user_login 接收 user 和 passwd。
- [ ] 正确凭证返回 error=0、uid、host、port、token。
- [ ] 错误凭证返回 1009。
- [ ] MySQL 查询使用 PreparedStatement，没有字符串拼接。
- [ ] Redis 中存在 user_token_<uid>，TTL 接近 3600 秒。
- [ ] Redis 或配置失败不会返回 error=0。
- [ ] 登录成功前没有调用 TCP ChatServer。

### Qt

- [ ] 点击登录只产生一次 POST /user_login。
- [ ] 网络错误和 GateServer 业务错误显示不同提示。
- [ ] 正确登录后保存四个交接字段。
- [ ] Qt 日志不打印密码和 token。
- [ ] 登录成功后没有提前调用 QTcpSocket::connectToHost()。

### 手工验收路径

1. 启动 chatmysql、chatredis、VerifyServer、GateServer；
2. 确认至少有一个已注册 User；
3. 用 curl 先验证正确和错误凭证；
4. 用 Redis EXISTS、TTL、STRLEN 验证 token；
5. 启动 Qt，输入同一组凭证；
6. 确认弹出登录成功提示，且 Qt 日志只显示 uid、host、port；
7. 确认 9000 没有 TCP 连接；这是下一课的工作，不是本课失败；
8. 重启 GateServer 后重复登录，确认仍能得到新的 token；
9. 等待或手工缩短 TTL 做过期观察，但不要修改源码来绕过 TTL。

## 11. 面试版讲解

登录阶段我把认证和连接建立拆开了。Qt 先通过 HTTP 把用户名和密码提交给 GateServer，GateServer 通过 MySQLDAO 的参数化查询得到 User 的 uid。凭证正确后，GateServer 生成一个短时效 token，并以 user_token_<uid> 为 key 写入 Redis，同时从配置读取唯一 ChatServer 的 host 和 port，最后把 uid、地址和 token 返回给 Qt。

Qt 收到响应后只保存交接数据，还没有建立 TCP 连接。这样 HTTP 登录成功和 TCP 连接建立是两个可独立验证的阶段；下一课再用 uid 和 token 接入 TCP ChatServer。当前 demo 的 token、明文密码和非 TLS HTTP 都不是生产级安全方案，课程只是用它们展示服务边界和生命周期。

如果登录失败，我会先判断是客户端网络层、GateServer 路由、MySQL 凭证查询、Redis token 写入还是 Qt 回包分发出了问题，而不是把所有失败都显示成“登录失败”。

## 12. 自查问题

1. 为什么登录不能调用 RegUser()？
2. 为什么 token 的 Redis key 使用 uid，而不是直接使用密码或邮箱？
3. 为什么 error=0 之后还不能说明 TCP ChatServer 已连接？
4. InvalidCredentials 和 QueryFailed 为什么必须分开？
5. 为什么 user、passwd 是 JSON 字段，而 SQL 使用 name、pwd？
6. 为什么 HttpManager 要按 Modules 和 ReqId 分两次分发？
7. 如果 Redis 写入成功但 JSON 回包发送失败，下一次登录会发生什么？
8. 为什么当前 xorString 既不安全又可能让正确密码变错？
9. 为什么本课不创建 StatusServer？
10. 如何证明 token 已写入 Redis，但又不把 token 打印到日志？
11. 为什么本课不把 uid 作为客户端请求字段？

## 13. 本课与后续课程的边界

本文件已经从 draft 扩写为 ready；学习者完成真实 curl 和 Qt 验收、提交结果后，才能变为 closed。

本课不实现：

- TCP ChatServer；
- 4 字节 TCP 帧；
- token 在 TCP 服务端的校验；
- ChatServer Session；
- 登录后主窗口和聊天列表；
- StatusServer、动态服务发现或多 ChatServer。

这些内容分别属于第 03、04 课及后续课程。保持边界，才能让每一课都有一个短而可验证的闭环。

---
status: backend-verified-qt-manual-pending
day: 1
scope: GateServer registration baseline and HTTP login
source_of_truth: current repository
---

# Day1：注册与 HTTP 登录链路（实现完成，按教程复盘）

## 今天的结果

当前工作区已经完成后端实现，并完成了 GateServer 构建、注册/登录接口、错误密码和 Redis TTL 验收。Qt 登录交接代码也已编译；下面代码块用于照教程复盘，手工打开 Qt 验收后就进入 TCP。

完成后用 curl 应能观察到：

~~~json
{
  "error": 0,
  "uid": 5,
  "host": "127.0.0.1",
  "port": 9000,
  "token": "<每次登录都不同>"
}
~~~

错误密码必须返回 1009。成功时 Redis 中必须出现 user_token_<uid>，并且有 3600 秒左右的 TTL。

教程参考章节是“框架概述和登录界面设计”“Beast 实现 HTTP 服务器”“处理 POST 请求并解析 JSON”“Redis 服务搭建”“注册功能”“登录和状态服务”。教程中的 Node.js 邮箱服务改用当前仓库已经存在的 Go VerifyServer。

## 0. 建立当天工作点

在 WSL 的 zsh 中执行：

~~~zsh
cd /home/deadnumb/C++ChatProject
git switch -c learn/season1-day1-login
git status --short
~~~

如果分支已经存在，使用：

~~~zsh
git switch learn/season1-day1-login
~~~

工作区应当没有你不认识的改动。不要用旧博客文件覆盖整个 apps/gate-server。

## 1. 先看现状，再复制

~~~zsh
cd /home/deadnumb/C++ChatProject
grep -n "CheckLogin\|LoginCheckResult" apps/gate-server/include/MySQLDao.h apps/gate-server/src/MySQLDao.cpp
grep -n "CheckEmailExists\|RegUser" apps/gate-server/include/MySQLManager.h apps/gate-server/src/MySQLManager.cpp
grep -n "user_register\|get_verifycode\|user_login" apps/gate-server/src/LogicSystem.cpp
~~~

当前工作区已经包含这些实现：

- LoginCheckResult 和 CheckLogin 已经接通；
- MySQLManager 已经转发登录查询；
- LogicSystem 已经提供 /user_login；
- token 已写入 Redis，并设置约 3600 秒 TTL。

如果要练习“从空桩补齐”，请先切出练习分支，再复制本文件标注的代码块。代码块按当前源码接口写，避免把教程中的旧目录和旧类名整段粘过来。

## 2. 复制粘贴一：实现 MySQL 登录查询

打开 apps/gate-server/src/MySQLDao.cpp，找到文件末尾现有的空函数，把整个函数替换成下面这段：

~~~cpp
LoginCheckResult MySQLDAO::CheckLogin(const std::string& name,
                                      const std::string& pwd,
                                      int& uid)
{
    uid = 0;

    auto con = pool_->getConnection();
    if (con == nullptr) {
        return LoginCheckResult::QueryFailed;
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
            return LoginCheckResult::InvalidCredentials;
        }

        uid = res->getInt("uid");
        pool_->returnConnection(std::move(con));
        return LoginCheckResult::Success;
    } catch (const sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException while checking login: "
                  << e.what()
                  << " (MySQL error code: " << e.getErrorCode()
                  << ", SQLState: " << e.getSQLState() << ")"
                  << std::endl;
        return LoginCheckResult::QueryFailed;
    }
}
~~~

这段代码做四件事：

1. 从连接池取连接；
2. 用 PreparedStatement 查询用户名和密码；
3. 没有匹配行时返回 InvalidCredentials；
4. 只有查询成功才把 uid 写回，并在所有路径归还连接。

当前数据库仍使用教程已有的明文 pwd 字段。今天先完成调用链，密码哈希放到后续加固。

## 3. 复制粘贴二：让 MySQLManager 转发能力

打开 apps/gate-server/include/MySQLManager.h，在 CheckEmailExists 后增加：

~~~cpp
LoginCheckResult CheckLogin(const std::string& name,
                            const std::string& pwd,
                            int& uid);
~~~

打开 apps/gate-server/src/MySQLManager.cpp，在 CheckEmailExists 实现后增加：

~~~cpp
LoginCheckResult MySQLManager::CheckLogin(const std::string& name,
                                          const std::string& pwd,
                                          int& uid) {
    return dao_.CheckLogin(name, pwd, uid);
}
~~~

保存后先不继续粘贴，执行一次构建。这样能确认你理解了“LogicSystem 依赖 Manager，Manager 依赖 DAO”的层次。

## 4. 复制粘贴三：在 LogicSystem 增加开发 token

打开 apps/gate-server/src/LogicSystem.cpp，在已有 VerifyGrpcClient include 后增加：

~~~cpp
#include <cstddef>
#include <random>
~~~

在 static const std::string CODEPREFIX = "verify_code_"; 后增加：

~~~cpp
namespace {

constexpr char TOKEN_ALPHABET[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr std::size_t TOKEN_LENGTH = 32;
const std::string TOKEN_PREFIX = "user_token_";
const auto TOKEN_TTL = std::chrono::seconds(3600);

std::string GenerateLoginToken() {
    static thread_local std::mt19937_64 engine(std::random_device{}());
    std::uniform_int_distribution<std::size_t> distribution(
        0, sizeof(TOKEN_ALPHABET) - 2);

    std::string token;
    token.reserve(TOKEN_LENGTH);
    for (std::size_t i = 0; i < TOKEN_LENGTH; ++i) {
        token.push_back(TOKEN_ALPHABET[distribution(engine)]);
    }
    return token;
}

}  // namespace
~~~

文件原来已经使用 std::chrono；如果编译器提示未声明，再在 include 区增加：

~~~cpp
#include <chrono>
~~~

这个 token 只是本地教学凭证，没有 TLS、密钥轮换或生产级安全承诺。

## 5. 复制粘贴四：注册 /user_login

仍在 LogicSystem::LogicSystem() 中，放在 RegPost("/user_register", ...) 前面：

~~~cpp
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

        auto& config = ConfigManager::get_instance();
        const auto chat_host = config["ChatServer"]["Host"];
        const auto chat_port_text = config["ChatServer"]["Port"];

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

        const auto token = GenerateLoginToken();

        try {
            const bool stored = (*RedisManager::get_instance())->set(
                TOKEN_PREFIX + std::to_string(uid),
                token,
                TOKEN_TTL);
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
        response["host"] = chat_host;
        response["port"] = chat_port;
        response["token"] = token;

        beast::ostream(connection->_response.body())
            << response.toStyledString();
    });
~~~

粘贴后确认 config.example.ini 里已有：

~~~ini
[ChatServer]
Host = 127.0.0.1
Port = 9000
~~~

ConfigManager 按进程当前目录读取 config.ini。构建脚本会把示例配置复制到 out/build/gate-server-debug/bin/config.ini。

如果 redis-plus-plus 版本不接受带 TTL 的 set 重载，把 try 块中的写入替换成：

~~~cpp
            const auto key = TOKEN_PREFIX + std::to_string(uid);
            const bool stored = (*RedisManager::get_instance())->set(
                key, token);
            if (!stored
                || !(*RedisManager::get_instance())->expire(key, 3600)) {
                write_error(ErrorCode::RedisError);
                return;
            }
~~~

优先使用单次带 TTL 的写入。

## 6. 复制粘贴五：去掉注册响应中的敏感字段

当前 /user_register 成功分支把 passwd、confirm 和验证码原样放进 HTTP 响应。找到并删除：

~~~cpp
    root["passwd"] = pwd;
    root["confirm"] = confirm;
    root["varifycode"] = src_root["varifycode"].asString();
~~~

注册成功响应保留：

~~~cpp
    root["error"] = ErrorCode::Success;
    root["uid"] = uid;
    root["email"] = email;
    root["user"] = name;
~~~

这一步只停止把凭证回传给客户端和日志观察者，不能把当前 demo 描述成安全系统。

## 7. 复制粘贴六：修正连接池停机和归还

Day1 运行结束要能按 Ctrl-C 停止 GateServer。当前连接池的 Close 和 returnConnection 各有一个明显的生命周期问题。

在 apps/gate-server/src/MySQLDao.cpp 中，把：

~~~cpp
void MySQLPool::Close() {
    b_stop_ = false;
    cond_.notify_all();
}
~~~

替换为：

~~~cpp
void MySQLPool::Close() {
    if (b_stop_.exchange(true)) {
        return;
    }

    cond_.notify_all();
    if (check_thread_.joinable()) {
        check_thread_.join();
    }
}
~~~

再把整个 returnConnection 替换为：

~~~cpp
void MySQLPool::returnConnection(std::unique_ptr<SQLConnection> con) {
    if (con == nullptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        pool_.push(std::move(con));
    }
    cond_.notify_one();
}
~~~

原函数创建了一个没有绑定到 mutex_ 的 unique_lock，没有真正保护连接池。这个修复只处理退出和归还路径，查询逻辑保持教程的简单形式。

## 8. 编译

先执行：

~~~zsh
cd /home/deadnumb/C++ChatProject
./scripts/dev/build-gate.sh
~~~

如果提示找不到 redis++，按脚本提示设置本机安装目录，例如：

~~~zsh
export REDIS_PLUS_PLUS_DIR="$HOME/Redis++/redis-plus-plus/build/cmake"
./scripts/dev/build-gate.sh
~~~

预期产物：

~~~text
out/build/gate-server-debug/bin/GateServer
out/build/gate-server-debug/bin/config.ini
~~~

只编译通过还没有完成 Day1，继续做运行验收。

## 9. 启动本地依赖和 GateServer

Docker 已经有镜像时，在终端 A 执行：

~~~zsh
cd /home/deadnumb/C++ChatProject
docker compose up -d mysql redis
docker compose ps
./scripts/dev/run-gate.sh
~~~

run-gate.sh 会占用当前终端。看到 GateServer 监听 8080 后保持它运行。

如果 Docker 因网络拉不到镜像：

- 不要反复重试；
- 记录“Docker 镜像网络阻塞”；
- 如果已有 MySQL、Redis 或旧容器可用，继续做本地验收；
- 如果没有依赖，源码阅读和编译仍可完成，运行项标记为环境阻塞。

Day1 登录不需要启动 VerifyServer，因为 /user_login 只访问 MySQL、Redis 和配置文件。

## 10. curl 验收

另开终端 B，仍使用 zsh：

~~~zsh
cd /home/deadnumb/C++ChatProject
export CHAT_DEMO_USER='你已有的本地用户名'
export CHAT_DEMO_PASS='该账号密码'

response=$(curl -sS -X POST \
  -H 'Content-Type: application/json' \
  -d "{\"user\":\"$CHAT_DEMO_USER\",\"passwd\":\"$CHAT_DEMO_PASS\"}" \
  http://127.0.0.1:8080/user_login)

print -r -- "$response"
~~~

正确响应需要同时有 error=0、正数 uid、host=127.0.0.1、port=9000 和非空 token。

错误密码：

~~~zsh
bad_response=$(curl -sS -X POST \
  -H 'Content-Type: application/json' \
  -d "{\"user\":\"$CHAT_DEMO_USER\",\"passwd\":\"wrong-password\"}" \
  http://127.0.0.1:8080/user_login)

print -r -- "$bad_response"
~~~

预期 error=1009。这个请求不应产生新的成功 token。

知道 uid 后检查 Redis。读取你本机的开发配置，不要把真实密码写进命令历史：

~~~zsh
set -a
source /home/deadnumb/C++ChatProject/.env
set +a

docker compose exec -T redis redis-cli \
  -a "$REDIS_PASSWORD" TTL "user_token_<uid>"
~~~

预期 TTL 接近 3600，并且随时间递减。不要把完整 token 写进截图。

## 11. Day1 验收表

| 项目 | 操作 | 通过证据 | 状态 |
| --- | --- | --- | --- |
| DAO 查询 | 编译 GateServer | build-gate.sh 返回 0 | ☐ |
| 配置 | 启动 GateServer | 日志读取 config.ini，监听 8080 | ☐ |
| 正确登录 | curl 正确账号 | error=0，有 uid、host、port、token | ☐ |
| 错误密码 | curl 错误密码 | error=1009 | ☐ |
| token TTL | Redis TTL user_token_<uid> | TTL 约 3600 并递减 | ☐ |
| 注册回包 | 注册成功后查看 JSON | 没有 passwd、confirm、varifycode | ☐ |
| 停机 | Ctrl-C GateServer | 进程退出，无连接池线程崩溃 | ☐ |
| 解释 | 不看代码口述 | 能讲清 DAO→Manager→LogicSystem→Redis | ☐ |

只有“通过证据”已写下，才把 ☐ 改成 ☑。

## 12. 常见问题

| 现象 | 先查什么 | 结论 |
| --- | --- | --- |
| 编译找不到 redis++ | REDIS_PLUS_PLUS_DIR 和本地安装 | 工具链问题 |
| 启动找不到 config.ini | 当前目录是否是 out/build/gate-server-debug/bin | 启动目录问题 |
| MySQL 连接失败 | docker compose ps、端口 3308 | 依赖未就绪 |
| Redis 写入失败 | Redis 密码和 6380 映射 | 依赖或配置问题 |
| 正确密码返回 1009 | 数据库 user 表中的 name/pwd | 凭证或字段问题 |
| Ctrl-C 后异常退出 | 是否复制了第 7 节连接池修复 | 生命周期问题 |
| Docker 拉镜像失败 | 网络、镜像缓存、Compose 日志 | 环境阻塞，不改 C++ 代码 |

## 13. 给自己讲一分钟

“客户端把用户名和密码通过 HTTP 发给 GateServer。LogicSystem 负责请求解析和响应编排，MySQLManager 再把查询交给 MySQLDAO。DAO 用参数化 SQL 查询 uid，并区分凭证错误和数据库错误。凭证成功后，GateServer 生成短时 token 写入 Redis，再从配置返回未来 ChatServer 的地址。HTTP 登录成功只代表拿到了 TCP 交接数据，尚未建立聊天连接。”

## 14. 完成线

完成本文件的验收表后，才进入 Day2。Day2 直接使用今天得到的 JSON 契约，不重新设计后端字段。

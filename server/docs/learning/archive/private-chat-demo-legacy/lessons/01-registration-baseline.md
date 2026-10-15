---
status: closed
estimated_hours: 3
blocked_by: none
expanded_at: 2026-08-28
closed_at: 2026-08-28
based_on: 2026-08-28 当前代码快照（WSL 后端 + D:\QTChat\QTChat）
implementation: 已落地独立邮箱查重、1007/1008 错误边界、Qt 提示和重复邮箱回归；关闭 Clash TUN 后真实邮箱、Qt 注册、MySQL 落库和重复请求均已验收
---

# 第 01 课：端到端跑通注册流程

## 0. 先读这段：本课记录的是什么

本课不是让你重新修改一份“仍然坏着”的代码。当前实现已经把注册链路的主要断点落地了，本课负责把它读懂、验证，并沉淀故障定位方法：

1. 读懂 GateServer、VerifyServer、Redis、SMTP、MySQL 和 Qt 的职责边界；
2. 用可重复的命令验证当前真实代码；
3. 解释一次真实故障，而不是把“接口成功”误说成“邮件送达”。

因此，下面出现“历史问题”时，指的是上一个实现提交之前的状态；出现“当前”时，以仓库里的真实代码为准，不要重复粘贴已经存在的修改。

本课已标记 `closed`：2026-08-28 已完成真实邮箱收信、Qt 手动注册、MySQL 落库和重复邮箱回归。MySQL 连接池并发/生命周期问题仍是后续加固项，不把本课的单次成功包装成生产级稳定性。

## 1. 完成后你会看到什么

- GateServer 会在发送验证码前只读检查邮箱；已注册邮箱返回 `1007`，不会调用 VerifyServer；
- VerifyServer 通过 gRPC 生成验证码，写入 Redis 180 秒，并异步尝试通过 QQ SMTP 发送；
- GateServer 从 Redis 校验验证码，再通过 MySQL 存储过程创建 User；
- Qt 能区分重复邮箱、重复注册、验证码错误、验证码过期和数据库异常；
- 你能说明 Redis 写码、SMTP 接受和 Outlook 最终收信不是同一个结果；
- 你能用 STAR 结构讲清楚一次从现象到根因、修复和验收的故障故事。

验证码写入 Redis 成功，只代表服务端已经保存验证码，不代表 SMTP 已接受，更不代表 Outlook 最终收信。当前课程要求真实邮箱验收；开发排障时仍要分别看 Redis、VerifyServer SMTP 日志和邮箱。

## 2. 系统调用链

当前已经落地的链路是：

```
Qt RegisterDialog
  └─ HTTP POST /get_verifycode
       └─ GateServer LogicSystem
            ├─ MySQL 只读检查 email
            │    ├─ Exists → 返回 GateServer `1007`，停止
            │    ├─ QueryFailed → 返回 `1008`，停止
            │    └─ NotFound → 继续
            └─ gRPC VerifyService.GetVerifyCode
                 ├─ 生成 6 位验证码
                 ├─ Redis SET verify_code_<email> EX 180
                 └─ 异步 SMTP 发邮件

Qt RegisterDialog
  └─ HTTP POST /user_register
       └─ GateServer LogicSystem
            ├─ Redis GET verify_code_<email>
            ├─ 校验密码、验证码
            └─ MySQL CALL reg_user(?,?,?,@result)
                 └─ user 表新增 User
```

错误码边界也已经明确：VerifyServer 内部使用 `0/1/2`；GateServer 将上游失败翻译为 `1002`，将本地查重和数据库异常分别翻译为 `1007/1008`；Qt 只处理 GateServer 的业务错误码。

这里有两个故意分开的查重动作：`CheckEmailExists` 是发送验证码前的纯查询，`RegUser` 是验证码校验后的“查重并注册”存储过程。前者解决用户体验和无效发信，后者保留最终一致性防线；不能在发验证码前调用 `RegUser`，因为它会产生注册副作用。

## 3. 当前实现拆解

| 位置 | 已落地的改动 | 学习重点 |
| --- | --- | --- |
| `apps/gate-server/config.ini` | 补充 MySQL 的 `User`、`Schema`，统一 Redis、GateServer、VerifyServer 配置 | 配置键名区分大小写 |
| `apps/gate-server/src/RedisManager.cpp` | 读取 `Host`、`Port`、`Password` | 配置文件和代码必须逐字符一致 |
| `apps/gate-server/src/GateServer.cpp` | 使用 `[GateServer]/Port`，不再硬编码 8080 | 配置驱动端口 |
| `apps/verify-server/main.go` | 启动时调用 `InitRedis()`；生成验证码后写入 Redis，TTL 180 秒 | gRPC 服务依赖 Redis 的初始化顺序 |
| `apps/verify-server/const.go` | `CodePrefix` 统一为 `verify_code_` | 两端键名必须一致 |
| `apps/gate-server/src/MySQLDao.cpp` | `CheckEmailExists()` 使用参数化查询返回 `NotFound/Exists/QueryFailed` | 纯查询和有副作用的注册操作分开 |
| `apps/gate-server/src/LogicSystem.cpp` | `/get_verifycode` 先查邮箱；`/user_register` 继续调用 `reg_user` | 前置体验检查不替代最终数据防线 |
| `apps/gate-server/include/global.h` | 增加 `EmailExist=1007`、`DatabaseError=1008` | 服务端业务错误要有稳定契约 |
| `QTChat/global.h` | 增加对应 `GateErrorCode` | 客户端和服务端错误码一一对应 |
| `QTChat/registerdialog.cpp` | 重复邮箱提示“该邮箱已注册，请直接登录” | 用户能理解为什么不能继续 |
| `apps/verify-server/email.go` | `SendMail()` 使用 goroutine，记录 SMTP 成功/失败 | 异步返回不能伪装成最终送达 |
| `apps/gate-server/mysql/init/01_init.sql` | 提供 `user` 表和 `reg_user` 存储过程骨架 | 数据库约束和存储过程契约 |

这些改动已经存在于当前工作区。阅读它们时，请把重点放在“为什么要这样改”和“改动如何连接起来”，不要再次执行同样的替换。

## 4. 开工前检查

以下命令要在 WSL 的 zsh 登录环境中执行。Go 是手动安装在 `/usr/local/go`，所以不要用未加载 `.zshrc` 的普通非登录 Bash 判断环境。

```bash
zsh -lic 'go version'
# 预期：go version go1.26.3 linux/amd64，版本号以本机实际输出为准

zsh -lic 'command -v docker && docker ps -a --format "{{.Names}} {{.Status}}" | grep -E "^(chatmysql|chatredis)($| )"'
# 预期：能看到 chatmysql、chatredis；Exited 也可以，后面会启动
```

先做网络前置检查。VerifyServer 使用 QQ SMTP 的 465 端口；Clash TUN 的 Fake-IP
可能让 `smtp.qq.com` 解析到 `198.18.*`，导致 Go 的直接 TLS 连接被重置或超时。

```bash
getent ahostsv4 smtp.qq.com
timeout 8 openssl s_client \
  -connect smtp.qq.com:465 \
  -servername smtp.qq.com \
  -brief </dev/null
```

如果看到 `198.18.*` 或 TLS 握手超时，验收时暂时关闭 TUN，或者把 SMTP
域名加入 Fake-IP 排除并设置直连。不要把“浏览器能上网”当成 SMTP 链路正常。

Windows Qt 使用 Qt Creator 的 MinGW Kit 构建。当前 MSVC 构建目录曾因工具链找不到 `type_traits` 失败，但 Qt Creator 的 `Desktop_Qt_6_9_0_MinGW_64_bit-Debug` 构建已经通过；不要混用两个 Kit 的构建目录。

## 5. 复核 Docker 和数据库

### 5.1 启动已有容器

```bash
docker start chatmysql chatredis

# 从本机私密配置中填入，绝不要把真实密码写入课程、日志或截图。
export CHAT_MYSQL_PASSWORD='<本机 MySQL 密码>'
export CHAT_REDIS_PASSWORD='<本机 Redis 密码>'

for i in {1..30}; do
    if docker exec -e MYSQL_PWD="$CHAT_MYSQL_PASSWORD" \
        chatmysql mysqladmin ping -h127.0.0.1 -uroot --silent >/dev/null 2>&1; then
        echo "mysql-ready"
        break
    fi
    sleep 1
done

docker exec -e REDISCLI_AUTH="$CHAT_REDIS_PASSWORD" chatredis redis-cli ping
# 预期：PONG
```

端口映射是 MySQL `3308:3306`、Redis `6380:6379`，必须与 `out/build/gate-server-debug/bin/config.ini` 中的配置一致。
课程命令使用环境变量传递凭证；如果本地配置中的密码不同，以本机配置为准，不要修改源码来“适配”教学文本。

### 5.2 检查数据库，不要盲目重建

```bash
docker exec -e MYSQL_PWD="$CHAT_MYSQL_PASSWORD" chatmysql mysql -uroot -e \
  "USE chatdb; SHOW TABLES; SHOW PROCEDURE STATUS WHERE Db='chatdb'; DESCRIBE user;"
```

预期至少看到 `user` 表和 `reg_user` 存储过程。`01_init.sql` 中使用 `CREATE TABLE IF NOT EXISTS`，它适合初始化，不是迁移工具；如果已有旧表结构，重新执行脚本不会自动替换旧表。只有确认是本地演示库、数据可以丢失时，才按单独的开发数据库重建流程处理。

上一个实现阶段遇到过旧表结构和排序规则冲突，表现为 `Result:-1` 被 GateServer 误报成 `1004`。当前验收使用的数据库已经是 `uid/name/email/pwd` 结构，存储过程可以正常返回新 uid；不要把“脚本执行过”误认为“数据库一定已经迁移完成”。

## 6. 阅读当前实现的关键代码

### 6.1 VerifyServer：先存 Redis，再异步发信

打开 `apps/verify-server/main.go`：

- `main()` 在 `InitEmail()` 后调用 `InitRedis()`；
- `GetVerifyCode()` 生成验证码；
- `RedisCli.Set(ctx, CodePrefix+email, code, 180*time.Second)` 保存验证码；
- Redis 写入失败时返回 `RedisErr`，成功后才异步调用 `SendMail`。

关键代码应保持如下逻辑，注意 `textStr` 必须在 `SendMail` 前定义：

```go
code := generateCode()

if RedisCli == nil {
    return &pb.GetVerifyRsp{Email: email, Error: RedisErr}, nil
}
if err := RedisCli.Set(ctx, CodePrefix+email, code, 180*time.Second).Err(); err != nil {
    return &pb.GetVerifyRsp{Email: email, Error: RedisErr}, nil
}

textStr := fmt.Sprintf("您的验证码为 %s，请三分钟内完成注册", code)
SendMail(email, "验证码", textStr)
```

`apps/verify-server/const.go` 中的 `CodePrefix` 必须是 `verify_code_`，因为 GateServer 的注册逻辑使用同一个前缀读取 Redis。

当前 `SendMail()` 的语义是“启动一个后台发送任务”，不是“等待邮件送达”：

```go
func SendMail(to, subject, body string) {
    go func() {
        m := gomail.NewMessage()
        m.SetHeader("From", EmailUser)
        m.SetHeader("To", to)
        m.SetHeader("Subject", subject)
        m.SetBody("text/html", body)
        if err := mailDialer.DialAndSend(m); err != nil {
            log.Printf("发送邮件失败 [%s]: %v", to, err)
        } else {
            log.Printf("邮件发送成功 [%s]", to)
        }
    }()
}
```

所以 gRPC 的 `error: 0` 只表示 Redis 写码成功并启动了发信任务。SMTP 失败会在之后写日志，不能回到已经返回的 gRPC 响应中。

### 6.2 GateServer：先查重，再调用 VerifyServer

打开 `apps/gate-server/src/LogicSystem.cpp` 的 `/get_verifycode` handler。发送验证码前的关键逻辑是：

```cpp
const auto email_check =
    MySQLManager::get_instance()->CheckEmailExists(email);

if (email_check == EmailCheckResult::Exists) {
    root["error"] = ErrorCode::EmailExist;      // 1007
    return true;
}
if (email_check == EmailCheckResult::QueryFailed) {
    root["error"] = ErrorCode::DatabaseError;   // 1008
    return true;
}

// 只有 NotFound 才调用 VerifyServer
GetVerifyRsp rsp =
    VerifyGrpcClient::get_instance()->GetVerifyCode(email);
```

`CheckEmailExists()` 使用参数化的 `SELECT 1 ... LIMIT 1`，只是查询，不会创建 User。`RegUser()` 则调用 MySQL 存储过程：

```cpp
CALL reg_user(?,?,?,@result)
SELECT @result AS result
```

存储过程返回 `0` 表示用户名或邮箱重复，返回新 `uid` 表示成功，DAO/数据库异常返回 `-1`。因此两处查重不是重复实现，而是两个不同阶段的职责：发送前改善体验，注册时守住最终一致性。

GateServer 再把 VerifyServer 内部错误码翻译成客户端错误码：

```cpp
root["error"] = (rsp.error() == ErrorCode::Success)
    ? ErrorCode::Success
    : ErrorCode::RPCFailed;
```

这里不是简单透传 VerifyServer 的内部数字，而是在服务边界转换错误域。注册接口自身返回的 `1003~1006`、发送前查重的 `1007` 和数据库异常的 `1008` 都属于 GateServer 业务错误码。

### 6.3 Qt：异步回包必须映射到 handler

打开 `QTChat/registerdialog.cpp`：

- `ID_GET_VERIFY_CODE` handler 处理验证码响应；
- `ID_REG_USER` handler 处理注册成功、重复用户、密码不一致、验证码错误、验证码过期和数据库异常；
- `GATE_EMAIL_EXIST = 1007` 时显示“该邮箱已注册，请直接登录”，阻止用户继续等待邮件；
- `slot_reg_mod_finish()` 解析 JSON 后通过 `_handlers[id]` 分发。

打开 `QTChat/global.h`，确认 `GateErrorCode` 与 GateServer 的 `ErrorCode` 一一对应。`ReqId::ID_REG_USER = 1002` 是请求编号，不是业务错误码，二者不要混淆。

## 7. 构建和启动

### 7.1 构建 VerifyServer 和 GateServer

```bash
# VerifyServer：zsh 登录环境中使用手动安装的 Go
cd /home/deadnumb/C++ChatProject/apps/verify-server
go test ./...
go build -o verify-server .

# GateServer：CMake 已经把产物输出到 out/build/gate-server-debug/bin，不需要 cp
cd /home/deadnumb/C++ChatProject/apps/gate-server/build
cmake ..
cmake --build . --parallel 2
```

预期：

- Go 测试返回 `[no test files]` 或实际测试通过；
- C++ 显示 `Built target ChatServer`；
- 可执行文件位于 `/home/deadnumb/C++ChatProject/apps/gate-server/bin/ChatServer`。

### 7.2 启动顺序

终端一：

```bash
cd /home/deadnumb/C++ChatProject/apps/verify-server
./verify-server
```

预期日志包含 Redis 连接成功和 `监听 0.0.0.0:50051`。

终端二：

```bash
cd /home/deadnumb/C++ChatProject/apps/gate-server/bin
./ChatServer
```

预期日志最后包含 `Port listen to: 8080`。GateServer 的 `config.ini` 必须位于二进制同目录，因为 `ConfigManager` 按当前工作目录读取配置。

## 8. 后端命令行验收

VerifyServer 没有开启 gRPC reflection，因此使用 `grpcurl` 时必须提供 proto 文件：

```bash
cd /home/deadnumb/C++ChatProject/apps/verify-server
/home/deadnumb/go/bin/grpcurl -plaintext \
  -import-path /home/deadnumb/C++ChatProject/apps/verify-server/proto \
  -proto message.proto \
  -d '{"email":"你的真实邮箱"}' \
  127.0.0.1:50051 message.VerifyService/GetVerifyCode
```

响应中的 `error` 为 0 后，立即检查 Redis：

```bash
docker exec -e REDISCLI_AUTH="$CHAT_REDIS_PASSWORD" chatredis redis-cli KEYS 'verify_code_*'
docker exec -e REDISCLI_AUTH="$CHAT_REDIS_PASSWORD" chatredis redis-cli TTL 'verify_code_<你的邮箱>'
```

预期能看到验证码键，TTL 接近 180 秒并持续递减。开发环境如果暂时不验证邮箱投递，可以从 VerifyServer 日志中读取验证码；最终验收仍应使用真实邮箱。

如果邮箱已经注册，直接验证发送前查重：

```bash
cd /home/deadnumb/C++ChatProject/apps/gate-server
bash issue/duplicate-email-get-verifycode.sh
```

预期输出：

```text
PASS: duplicate email was rejected before verification-code issuance
```

这个回归不仅检查响应是 `1007`，还检查原有 Redis 验证码没有被覆盖。它覆盖的是实际 HTTP 接口，而不是孤立调用某个 DAO 函数。

用验证码替换下面的占位符后测试注册：

```bash
curl -sS -X POST \
  -H 'Content-Type: application/json' \
  -d '{"user":"alice_demo","email":"你的邮箱","passwd":"123456","confirm":"123456","varifycode":"验证码"}' \
  http://127.0.0.1:8080/user_register
```

成功响应的关键字段是 `error: 0` 和新的 `uid`。GateServer 日志中的 `Result` 应该是这个新 uid，不要固定写成 `1`。

### 8.1 负向用例

用刚才的账号和验证码重复注册，预期返回：

```json
{"error":1004}
```

使用同一个邮箱但把验证码改错，预期返回 `1005`。测试过期分支时，只对本地演示 Redis 写入一个 1 秒键，等待它过期后再提交，预期返回 `1006`；不要为测试目的重建整个数据库。

重复邮箱再次点击“获取验证码”不是 `1004`，而是发送前的 `1007`；`1004` 是验证码校验通过后，最终调用 `reg_user` 时发现用户名或邮箱重复的兜底结果。数据库连接或存储过程异常应返回 `1008`，不要继续误报为用户已存在。

### 8.2 如何判断“邮件没收到”卡在哪里

按下面的顺序切分链路，不要只看 Qt 的成功提示：

| 检查点 | 观察结果 | 结论 |
| --- | --- | --- |
| GateServer HTTP 响应 | `error=1007` | 已注册，未发验证码 |
| VerifyServer 日志 | `收到请求`、`生成验证码` | 请求进入验证码服务 |
| Redis | `verify_code_<email>` 存在，TTL 约 180 秒 | 验证码已保存 |
| VerifyServer 日志 | `邮件发送成功` | SMTP 已接受提交，不等于最终收件箱到达 |
| VerifyServer 日志 | `connection reset by peer` 或超时 | SMTP 网络/TLS/代理路径失败 |
| Outlook | 收件箱、垃圾邮件、其他文件夹 | 检查最终投递和过滤 |

本次真实阻塞属于最后两项之前的 SMTP 网络阶段：Clash TUN 开启时，`smtp.qq.com` 被解析到 `198.18.*` Fake-IP，465 端口 TLS 握手失败；关闭 TUN 后日志出现“邮件发送成功”，并且邮箱实际收到验证码。

## 9. Qt 手动验收

在 Qt Creator 中使用 MinGW Kit 构建 `QTChat`，确认构建输出目录中有 `config.ini`。当前 Qt 代码从 `applicationDirPath()` 读取配置，配置内容应为：

```ini
[GateServer]
host=127.0.0.1
port=8080
```

验收邮件前先确认 Clash TUN 已关闭，或已经为 `smtp.qq.com` 配置 Fake-IP 排除和直连。
这不是 Qt 代码问题，而是 VerifyServer 的直接 SMTP 连接会受到 WSL DNS 和路由影响。

启动 Qt 客户端后按顺序操作：

1. 使用未注册邮箱点击「获取验证码」；
2. 确认 VerifyServer 日志先出现“生成验证码”，随后出现“邮件发送成功”；
3. 在 Outlook 收件箱、垃圾邮件和其他文件夹确认收到验证码；
4. 填写用户名、密码、确认密码和验证码，点击确认；
5. 确认界面显示注册成功，并在 MySQL 的 `user` 表查到新的 `uid/name/email`；
6. 对刚注册的邮箱再次点击「获取验证码」，确认显示“该邮箱已注册，请直接登录”；
7. 使用同一用户名但不同新邮箱完成验证码后注册，确认显示“用户名或邮箱已存在”；
8. 输入错误验证码，确认显示“验证码错误”；
9. 使用新邮箱获取验证码，等待 TTL 过期后提交，确认显示“验证码已过期”。

Windows 到 WSL 的 `127.0.0.1` 转发通常可用，但不要把它当成无需验证的绝对前提。如果 Qt 报网络错误，先在 Windows PowerShell 执行 `Test-NetConnection 127.0.0.1 -Port 8080`，再检查 Qt 输出目录中的 `config.ini`。
Qt 显示“验证码已经发送到邮箱”只代表 GateServer 收到了 VerifyServer 的成功响应；最终邮件验收必须结合 VerifyServer 的 SMTP 日志和邮箱实际结果。

## 10. 当前验收记录（2026-08-28）

已经完成：

- [x] zsh 登录环境识别 Go 1.26.3；
- [x] `VerifyServer` 执行 `go test ./...`；
- [x] VerifyServer `go build` 通过；
- [x] GateServer CMake 构建通过；
- [x] Qt MinGW 构建通过；
- [x] gRPC 获取验证码成功；
- [x] Redis 验证码键存在，TTL 为 180 秒；
- [x] 首次注册成功并写入 MySQL；
- [x] 重复注册返回 1004；
- [x] 错误验证码返回 1005；
- [x] 过期验证码返回 1006；
- [x] 重复邮箱获取验证码返回 1007，且没有覆盖原 Redis 验证码；
- [x] Clash TUN 关闭后真实 SMTP 发送成功，真实邮箱收到验证码；
- [x] Windows Qt 客户端实际点击完成验证码注册；
- [x] MySQL 查到本次新 User 的 `uid/name/email`；
- [x] 注册后的重复邮箱请求仍返回 1007；
- [x] 验收产生的临时用户和 Redis 测试键已清理。

本次真实验收产生的 User 是故意保留的演示账号，不要把它当作临时测试数据删除。

尚未完成的后续加固：

- [ ] 修复 MySQLPool 的线程安全和退出问题；
- [ ] 清理密码日志、明文密码响应和配置中的敏感信息。

## 11. 已知问题与后续安排

| 问题 | 当前影响 | 处理安排 |
| --- | --- | --- |
| `MySQLPool::poolSize_` 未初始化 | 心跳线程存在未定义行为，注册成功不代表稳定 | 后续连接池专项修复 |
| `returnConnection()` 未锁成员 mutex | 与心跳线程并发访问连接队列时有竞态 | 后续连接池专项修复 |
| `Close()` 未正确停止心跳线程 | 进程退出可能无法正常回收 | 后续连接池专项修复 |
| `SendMail()` 是异步 goroutine | gRPC 成功不等于 SMTP 成功；失败只能晚些时候写日志 | Demo 保留；后续改为 outbox/队列和重试 |
| Clash TUN 的 Fake-IP 影响 SMTP | `smtp.qq.com` 解析到 `198.18.*` 时 465 TLS 失败 | 本机关闭 TUN，或配置 SMTP 域名直连 |
| Qt `xorString` 有返回值警告且不是真正加密 | 当前注册不受影响，登录时会遇到 | Issue 2 处理 |
| HTTP 日志和注册响应包含密码 | 本地 demo 可观察，但不能当生产方案 | Issue 10 统一清理 |
| 配置文件中存在 SMTP/数据库敏感信息 | 不应提交或展示给面试官 | 验收后轮换凭证并改环境注入 |

本课的 `closed` 只表示注册基线的可观察验收完成，不表示连接池、邮件可靠投递或安全性已经达到生产标准。

## 12. 面试版讲解

### 12.1 六十秒版本

我负责把一个 Qt 注册流程跑通。Qt 先通过 HTTP 调 GateServer，GateServer 在发码前用 MySQL 做只读邮箱查重，未注册时再通过 gRPC 调 VerifyServer。VerifyServer 把验证码写入 Redis 180 秒，并通过 QQ SMTP 发信。注册提交后，GateServer 从 Redis 校验验证码，再调用 `reg_user` 存储过程落库。

### 12.2 故事版本：验证码接口成功，但用户收不到邮件

**背景。** 我发现前端显示“验证码已发送”，但收件箱没有邮件。这个提示不能直接证明投递成功，因为系统当时只确认了 Redis 写码。

**任务。** 我需要把问题定位到具体边界，并保证重复邮箱不会触发无意义的发信，同时不能破坏最终注册的一致性防线。

**行动。** 我先用真实 HTTP 请求建立回归，再按 GateServer、VerifyServer、Redis、SMTP、邮箱逐层检查。数据库确认邮箱不存在，排除了 1007 查重。VerifyServer 有生成验证码记录，但 SMTP 报 `connection reset by peer`。

继续检查发现 Clash TUN 把 `smtp.qq.com` 解析到了 `198.18.*` Fake-IP，TLS 465 握手失败。关闭 TUN 后重新测试，日志出现 SMTP 成功，邮箱收到验证码。

代码上，我把发送前查重设计成独立的 `CheckEmailExists()`，已注册邮箱返回 `1007`，数据库异常返回 `1008`。注册提交仍调用 `reg_user`，以应对查重和注册之间的并发竞争。

**结果。** 新 User 成功写入 MySQL，Qt 注册成功；注册后的邮箱再次请求验证码返回 `1007`，Redis 中原验证码没有被覆盖。GateServer、VerifyServer、Redis、SMTP、MySQL 和 Qt 的核心路径都有证据。

**复盘。** 这次还暴露了异步发送的语义问题：接口先返回成功，SMTP 失败只能晚些时候写日志。Demo 阶段保留这个实现，但生产方案应使用 outbox 或消息队列、重试和可观测状态，不能把“已写 Redis”说成“邮件已送达”。

完整追问和简短回答见：[Issue 1 注册与验证码面试故事](../interview-story-01-registration.md)。

面试中不要说“邮件一定送达”或“系统已经生产级可靠”。更准确的表达是：系统记录了 Redis 写入和 SMTP 提交结果，真实邮箱在关闭本地 TUN 后收到；离线重试、投递回执和连接池并发安全仍属于后续加固。

## 13. 自查问题

1. 为什么发送验证码前不能复用 `RegUser()` 做查重？
2. `1007` 和 `1004` 分别发生在注册流程的哪两个阶段？
3. 为什么 `error: 0` 不能直接证明邮件已送达？
4. Clash TUN 为什么会影响 Go 的直接 SMTP 连接？
5. 为什么 `grpcurl` 调用这个服务时需要手动提供 `message.proto`？
6. `ReqId::ID_REG_USER` 和 `GateErrorCode::GATE_USER_EXIST` 分别属于什么概念？
7. 为什么这次注册成功，仍然不能说明 MySQLPool 没有并发问题？

## 功能实现与接口质量

本课实现顺序固定为：Qt 请求 → GateServer 校验 → VerifyServer 验证码 → MySQL 存储过程注册。函数优先返回值类型结果；不要用 `int&`、裸指针或全局变量传递 uid、错误码等结果。注册结果应明确区分成功、用户已存在和数据库失败。

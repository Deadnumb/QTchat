# C++ChatProject 学习区

这里是第一季聊天项目的学习入口。目标是用一周把教程主链路走完，并让每一天都能独立启动、观察和验收。

## 先看什么

1. [第一季路线图](season1/ROADMAP.md)
2. [Day1：注册基线与 HTTP 登录后端](season1/DAY-01-AUTH-BACKEND.md)
3. [一周验收表](season1/ACCEPTANCE.md)

旧的十课稿、注册复盘和早期登录稿保留在 archive/private-chat-demo-legacy，仅用于查资料。当前学习入口只看 season1。

## 当前项目边界

| 部分 | 当前事实 | 本周安排 |
| --- | --- | --- |
| C++ GateServer | 已有 /get_verifycode、/user_register、/user_login；注册和登录接口已做本地验收 | Day1 已完成 |
| Go VerifyServer | 已并入单仓库，负责 gRPC、Redis 验证码和 SMTP | Day1 复习 |
| MySQL / Redis | 由 Compose 提供本地开发依赖 | 每天启动前检查 |
| Qt 客户端 | 注册/登录 HTTP 交接代码已实现并编译，GUI 手工点击待验收 | Day2 收尾后进入 TCP |
| TCP ChatServer | 当前仓库还没有真正的聊天 TCP 服务 | Day3、Day6 |
| Docker | 网络不可用时可能无法拉镜像 | 本地能跑就继续，最后单独记录 |

源码能编译、HTTP 返回 200、进程处于运行状态，都只证明一个局部环节。当前后端注册/登录已有 curl 和 Redis 证据，Qt 界面点击仍需手工验收；TCP 仍未开始。

## 每天的固定动作

在 WSL 的 zsh 终端里执行：

1. 复制当天代码块；
2. 立即编译；
3. 启动当天需要的服务；
4. 做一条成功用例和一条失败用例；
5. 把结果写进一周验收表；
6. 只在验收通过后进入下一天。

交互命令统一按 zsh 写。仓库脚本保留自己的 shebang，仍从 zsh 调用。

## 配置和秘钥

- Git 中只放示例配置、开发占位值和 schema。
- 真实 SMTP 配置使用仓库外的 VERIFY_CONFIG_FILE 或本机私密备份。
- 真实密码、SMTP 密钥、完整 token 不写入 Markdown、源码、截图和 Git 历史。
- 当天登录使用专门的本地演示账号。

## 一周结束时要达到的闭环

两个 Qt 客户端能够：

1. 注册并登录；
2. 使用登录返回的 uid、host、port、token 建立 TCP 连接；
3. 搜索另一个用户；
4. 发送并同意好友申请；
5. 加载联系人；
6. 打开私聊；
7. 发送并在双方看到已持久化的文本消息。

心跳、严格至少一次投递、群聊、文件、TLS、高可用和多 ChatServer 留在后续章节。

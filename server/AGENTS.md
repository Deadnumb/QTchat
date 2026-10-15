# C++ChatProject 工作规则与学习交接

本文件是仓库唯一的 Agent 指南。详细教学放在 `docs/learning`，不要在这里复制整篇教程。

## 先读哪份文档

- 修改、构建或排查后端：先看本文件，再看当前源码。
- 复习注册和 HTTP 登录：看 [学习入口](docs/learning/README.md) 和 [Day1 操作稿](docs/learning/season1/DAY-01-AUTH-BACKEND.md)。
- 判断是否进入下一阶段：看 [第一季路线图](docs/learning/season1/ROADMAP.md) 和 [验收表](docs/learning/season1/ACCEPTANCE.md)。
- `docs/learning/archive` 只保存历史材料，不作为当前事实来源。

## 仓库边界

- `apps/gate-server`：C++ HTTP GateServer，负责注册、登录、MySQL、Redis 和 VerifyServer gRPC 调用。
- `apps/verify-server`：Go gRPC 验证码服务，负责 Redis 验证码和 SMTP。
- `contracts/proto`：跨服务 protobuf。
- `db/mysql/init`：MySQL 开发初始化脚本。
- 真正的 TCP ChatServer 尚未加入当前后端；GateServer 不是 TCP 服务。
- Windows Qt 工程位于仓库外：`D:\QTChat\QTChat`。不要用 WSL CMake 缓存编译它。

## 工具链和启动

- WSL 后端命令统一使用 zsh；工具链是 GCC、CMake、Ninja 和 Go。
- 常用命令：

```zsh
cd /home/deadnumb/C++ChatProject
docker compose up -d mysql redis
./scripts/dev/build-gate.sh
./scripts/dev/run-gate.sh
```

- GateServer 使用 `out/build/gate-server-debug/bin/config.ini`。
- Docker 拉镜像失败时记录环境阻塞，继续完成能做的源码和构建验收，不反复改业务代码。
- Qt 使用 Visual Studio Developer 环境构建，具体命令和手工验收见 docs/learning/season1/DAY-01-AUTH-BACKEND.md；Qt 工程的 CMake 配置是构建事实来源。

## 秘钥和配置

- Git 只保存示例配置、开发占位值和 schema。
- 真实 SMTP 配置放在仓库外，通过 `VERIFY_CONFIG_FILE` 或本机私密备份提供。
- 不把真实密码、SMTP 密钥或完整 token 写入源码、文档、日志和截图。
- 本地 `123456`、`day1_demo` 等只用于开发验收。

## 修改纪律

- 修改前先查看 `git status --short` 和目标文件。
- 保留开始工作前已有的未提交改动；不要使用 `git reset --hard`、`git clean` 或覆盖式复制。
- 没有用户要求时不提交 Git。
- 修改后至少执行 `git diff --check`，并按实际证据区分 implemented、staged、unverified。

## 当前交接状态

- GateServer 注册、登录、错误码和 Redis token 已实现并通过本地接口验收。
- Qt 注册/登录 HTTP 交接代码已编译；手工点击成功/失败提示仍由用户打开窗口确认。
- TCP ChatServer 尚未实现。
- 所以现在可以开始学习 TCP；不必继续扩展 HTTP 字段，也不必等待真实 SMTP。

## TCP 第一阶段：只做认证和一条完整帧

按这个顺序推进：

1. 新建 `apps/chat-server`，加入当前 CMake 构建。
2. 监听配置中的 `127.0.0.1:9000`。
3. Qt 登录成功后发送 `uid + token` 认证帧。
4. ChatServer 查询 Redis 的 `user_token_<uid>`。
5. token 正确返回认证成功帧，错误 token 返回失败并关闭连接。
6. 先完成一条带长度字段的回显帧，再做好友和聊天消息。
7. 验收必须覆盖：连接建立、Token 成功/失败、半包、连续两帧和 UTF-8 字节长度。

HTTP 登录成功只代表拿到了 TCP 交接数据；Qt 编译成功也不代表 TCP 已连接。

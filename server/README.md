# C++ChatProject

一个用于学习 C++ 后端、Go gRPC 和 Qt 客户端协作的聊天项目。

## 后端结构

- `apps/gate-server`：C++ HTTP GateServer，负责注册、登录、Redis、MySQL 和 VerifyServer gRPC 调用。
- `apps/verify-server`：Go gRPC 验证码服务，负责 Redis 验证码和 SMTP 发送。
- `contracts/proto`：跨服务 protobuf 源文件。
- `db/mysql/init`：开发环境数据库初始化脚本。
- `compose.yaml`：MySQL、Redis、VerifyServer、GateServer 的统一编排。

真正的 TCP ChatServer 会在后续单独加入，当前目录中的 GateServer 不承担 TCP 聊天连接。

## 支持环境

- 服务端开发：WSL Ubuntu 24.04
- C++：GCC 13、CMake、Ninja
- Go：Go 1.26
- 基础设施：Docker Compose
- 客户端：Windows Qt 项目，位于仓库外

交互终端可以使用 zsh；脚本通过 shebang 选择自己的解释器，不依赖 zsh 配置文件。

## 本地调试

首次准备环境变量：

```sh
cp .env.example .env
```

启动 MySQL 和 Redis：

```sh
./scripts/dev/up-infra.sh
```

构建 GateServer：

```sh
./scripts/dev/build-gate.sh
```

启动 VerifyServer。需要通过环境变量提供仓库外的配置文件：

```sh
export VERIFY_CONFIG_FILE="$HOME/.config/cpp-chat/secret-backup-<timestamp>/verify-config.json"
./scripts/dev/run-verify.sh
```

另开一个终端启动 GateServer：

```sh
./scripts/dev/run-gate.sh
```

默认端口：

| 服务 | 端口 |
|---|---:|
| MySQL | 3308 |
| Redis | 6380 |
| VerifyServer gRPC | 50051 |
| GateServer HTTP | 8080 |

## 全量 Compose

```sh
docker compose up -d --build
docker compose logs -f gate-server
```

镜像构建需要 Docker 能访问 Ubuntu、Go 和 Alpine 镜像源。网络不可用时，先使用上面的本地调试模式。

停止服务：

```sh
./scripts/dev/down.sh
```

## 配置

真实 SMTP 配置不进入 Git。VerifyServer 支持通过 `VERIFY_CONFIG_FILE` 指向仓库外的 JSON 配置文件；Compose 模式通过 `VERIFY_EMAIL_USER` 和 `VERIFY_EMAIL_PASS` 注入。

`config.example.ini` 和 `config.compose.ini` 只包含开发环境连接参数。

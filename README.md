# C++ IM

一个用于学习与梳理的即时通讯（IM）项目，包含 C++ 后端服务与 Qt 桌面客户端。目标是把注册、登录、好友、私聊这条主链路端到端跑通。

## 目录结构

```
client/   Qt6 桌面客户端（注册、登录、TCP 聊天管理）
server/   后端服务
  apps/gate-server    C++ HTTP GateServer（注册、登录、MySQL、Redis、gRPC 调用）
  apps/verify-server  Go 验证码服务（gRPC + Redis + SMTP）
  apps/status-server  C++ 状态服务（gRPC，分配 ChatServer + 生成登录 token）
  contracts/proto     跨服务 protobuf 定义
  db/mysql/init       MySQL 初始化脚本
```

## 技术栈

- 后端：C++20、Boost.Asio / Beast、gRPC、MySQL、Redis；Go（验证码服务）
- 客户端：Qt6（Widgets、Network）
- 基础设施：Docker Compose（本地 MySQL、Redis）

## 当前进度

- 已完成：注册、登录（HTTP 接口，登录下发 token 并写入 Redis）
- 进行中：TCP ChatServer（长连接认证与消息收发）、好友、私聊

## 构建与运行

- 后端：见 `server/AGENTS.md` 与 `server/docs/learning/`，WSL 下用 GCC + CMake + Ninja 构建。
- 客户端：见 `client/CMakeLists.txt`，用 Qt6 + CMake 构建。

## 说明

本项目为个人学习项目，仓库中只保留示例配置与开发占位值，真实密钥放在仓库外。

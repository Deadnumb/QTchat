---
status: draft
estimated_hours: 4
blocked_by: 02-http-login-handoff
---

# 第 03 课：搭建带帧的 TCP 消息循环

## 学习成果

Qt 连接到一个 C++ ChatServer，发送带帧的 JSON 测试消息，并在拆包和粘包的情况下都能收到正确组帧的响应。

## 教程必须覆盖

- 建立职责独立的 TCP ChatServer 及其可执行文件。
- 实现兼容教学项目的 4 字节头部和 8 KiB 正文上限。
- 按 UTF-8 字节数而不是字符数计长，保证中文正确组帧。
- 用 Boost.Asio 精确读取一个头部加一个正文。
- 在 Qt 端缓冲并解析一个或多个帧。
- 用每个 Session 的发送队列串行化写操作。
- 按消息 ID 分发，暂不加入聊天业务逻辑。

## 验收清单

- [ ] ASCII 和中文 JSON 都能正确往返。
- [ ] 一起发送的两个帧能被分开解析。
- [ ] 被拆开读取的帧能正确重组。
- [ ] 非法或超长长度能安全关闭连接。
- [ ] 服务端日志能区分 accept、读取、分发、写入和断开。

## 功能实现与接口质量

本课先完成 TCP 接收、帧重组、JSON 分发和响应回写，再扩展业务。解析函数返回值类型结果，例如 `ParseResult` 或 `std::optional<Frame>`；不要通过 `bool + int& length` 传出解析长度。所有权使用 `std::string`、`std::vector<std::byte>` 等值类型，非法长度必须在分配内存前拒绝。

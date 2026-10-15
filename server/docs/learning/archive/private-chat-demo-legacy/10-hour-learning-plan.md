# 10 小时 IM 学习与交付计划

## 目标

本计划的目标不是把在线聊天项目裁剪成一个只有 TCP 回显的 Demo，而是在有限学习时间内跑通教程中的主要 IM 闭环，并能够讲清楚核心模块的原理、设计取舍和常见故障。

编码由 Agent 辅助完成，学习者的时间主要用于：

- 理解调用链和状态变化；
- 阅读每个模块少量关键代码；
- 验收成功和失败路径；
- 准备面试表达。

## 范围原则

保留教程中能够体现 IM 基本功的功能：

- HTTP 登录到 TCP 长连接的交接；
- TCP 帧、拆包粘包、异步读写和发送队列；
- Session 认证、User 到 Session 的绑定和释放；
- LogicSystem 消息队列与业务回调分发；
- User 搜索、Friend Request、Friendship 和 Friend Contact；
- Private Conversation 建立；
- Text Message 持久化与在线推送；
- 登录后的会话列表和历史消息同步；
- 双客户端端到端演示。

根据现有 ADR，当前实现仍不加入 StatusServer、多 ChatServer 和跨服务器转发。它们是在线教程的分布式扩展，不是本期单服 IM 闭环的删减。

## 六个实施 Issue

### Issue 1：HTTP 登录交接

覆盖第 02 课，前置为第 01 课已关闭。

核心链路：

```text
Qt HTTP 登录
  → GateServer 查询 User
  → Redis 写入短时效 token
  → 返回 uid、host、port、token
```

学习者必须理解：HTTP 登录成功不等于 TCP 已连接；`uid` 是 GateServer 查询得到的服务端身份结果，不是登录表单的输入字段。

实现约束：使用结果类型表达登录成功和失败；不使用 `bool + int& uid`；不打印密码和完整 token；区分凭证错误、数据库错误和 Redis 错误。

验收：正确登录、错误密码、Redis 失败、重复登录、Qt 映射响应字段。

### Issue 2：TCP 帧协议与异步 Session

覆盖第 03 课。

核心链路：

```text
TCP accept
  → async_read_some
  → 接收缓冲区累积
  → 解析 4 字节头
  → 提取完整 JSON
  → 按消息 ID 分发
```

学习者必须理解 TCP 是字节流，单次读取不对应一条业务消息；长度必须按 UTF-8 字节计算；发送端需要为每个 Session 串行化异步写操作。

实现约束：拒绝非法和超长长度；解析函数返回值类型或 `optional`；不通过 `bool + int& length` 传出解析状态；发送队列保证同一 socket 不存在重叠写。

验收：ASCII、中文、半包、多包、超长包、非法包、连续多条响应。

### Issue 3：LogicSystem 与 TCP 认证

覆盖第 04 课，并吸收在线教程的逻辑层设计。

核心链路：

```text
Session 收到完整帧
  → LogicSystem 消息队列
  → 工作线程取出 LogicNode
  → 根据消息 ID 查找 handler
  → handler 校验 Redis/访问数据库
  → Session 返回响应
```

认证链路：

```text
TCP 连接
  → 提交 uid/token
  → Redis 校验 token
  → User 绑定到 Session
  → 允许业务消息
```

学习者必须理解条件变量、逻辑队列、回调注册、Session 生命周期、断开清理和未认证请求拦截。

实现约束：异步回调和 LogicNode 必须明确 Session 的存活保证；停服时工作线程能够退出；认证结果使用结果类型；不把所有错误都转换成“登录失败”。

验收：有效 token、无效 token、过期 token、未认证业务请求、正常断开、异常断开、重复认证。

### Issue 4：Friendship 业务闭环

覆盖第 05～08 课。

核心链路：

```text
搜索 User
  → 创建 Friend Request
  → 在线推送或重连加载
  → 接收方同意
  → 事务创建 Friendship
  → 创建两条 Friend Contact
  → 创建唯一 Private Conversation
  → 加载联系人列表
```

学习者必须区分 User、Friend Request、Friendship、Friend Contact、Private Conversation 和 Recent Conversation。

实现约束：禁止自己添加自己；重复请求和重复同意要有明确结果；同意流程在一个事务中完成；事务失败不能留下单边联系人；查询返回集合或空结果，不使用列表和数量的输出参数。

验收：搜索存在和不存在的 User、重复请求、自己请求自己、在线通知、断线后加载、事务回滚、双方得到相同的 Conversation ID。

### Issue 5：Text Message、Recent Conversation 与历史同步

覆盖第 09 课，并吸收在线教程聊天信息存储方案中的会话列表和游标分页。

在线消息链路：

```text
发送 Text Message
  → 认证与 Friendship 校验
  → Conversation 成员校验
  → 写入 chat_message
  → 返回“已持久化”
  → 推送在线接收方
  → 更新 Recent Conversation
```

重连同步链路：

```text
登录成功
  → 拉取 Private Conversation 列表
  → 按 thread_id 拉取消息
  → 使用 message_id 游标分页
  → 恢复聊天记录
```

学习者必须区分：服务端已持久化、已经推送到在线 Session、客户端确认收到和已读不是同一个状态。

本期保留离线消息的数据库保存与重新登录后的历史同步；不宣称已经实现完整送达 ACK、严格至少一次投递和复杂重试。

实现约束：数据库提交成功后才能向发送方返回成功；非好友不能发消息；消息 ID 和会话 ID 语义明确；分页返回游标和是否还有更多数据；数据库异常不能伪装成推送成功。

验收：中文消息、在线双端显示、接收方离线后重新登录、分页历史、首条消息进入 Recent Conversation、非好友发送被拒绝。

### Issue 6：Demo 加固、故障排查与面试彩排

覆盖第 10 课。

交付：启动与停止清单、服务故障分类、日志脱敏、资源释放检查、重复操作检查、双客户端演示视频或记录、面试讲稿。

至少能定位：HTTP 路由错误、Redis token 不存在、MySQL 查询失败、TCP 帧错误、Session 未认证、接收方离线、消息持久化失败和 SMTP/网络环境问题。

## 每个 Issue 的固定交付格式

Agent 不得只提交“代码已完成”。每个 Issue 必须同时提交：

1. 本 Issue 在整体调用链中的位置；
2. 输入、输出和状态变化；
3. 涉及的类、函数和数据表；
4. 关键接口的结果类型和错误域；
5. 所有权、线程和异步回调生命周期说明；
6. 成功路径和至少一个失败路径；
7. 构建、启动和验收命令；
8. 失败现象到根因的排障表；
9. 面试版回答；
10. 未实现内容和不能做出的生产级承诺。

## 10 小时学习安排

| 时间 | 重点 |
| --- | --- |
| 0.5 小时 | 阅读架构图、术语和 ADR，画出端到端调用链 |
| 1 小时 | HTTP 登录交接和 token 生命周期 |
| 2.5 小时 | TCP 帧、拆包粘包、异步读写和发送队列 |
| 1.5 小时 | LogicSystem、Session 认证和生命周期 |
| 2 小时 | Friend Request、事务、Friendship 和 Conversation |
| 1.5 小时 | 消息持久化、在线推送和历史同步 |
| 1 小时 | 故障排查、设计取舍和面试彩排 |

## 面试边界

可以讲：单 ChatServer 的 HTTP/TCP 分层、Asio 异步回调、TCP framing、Session、LogicSystem、MySQL 事务、消息持久化、在线推送和历史同步。

只能作为后续设计讲：StatusServer、负载均衡、多 ChatServer、跨服 RPC、心跳、完整 ACK、严格可靠投递、群聊和文件传输。

不能声称：生产级安全、高可用、严格 exactly-once、真实线上并发指标和未经过验收的稳定性。

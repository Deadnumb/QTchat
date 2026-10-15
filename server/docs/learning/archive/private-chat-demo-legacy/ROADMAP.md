---
status: ready
current_lesson: 02-http-login-handoff
deadline: 2026-09-01
daily_time_budget: 2 hours
---

# 在线私聊 Demo 学习路线图

> 面向 10 小时交付与面试复习的执行总览见：[10 小时 IM 学习与交付计划](10-hour-learning-plan.md)。本文件保留十课的教学顺序；实施时按学习闭环组织任务，不按源文件切碎任务。

## 我们要做什么

把这个面向新手的 C++ 聊天项目改造成一条有引导的学习路径，在约 10 个专注小时内产出一个可以拿出手演示的在线私聊 demo。该项目通过展示 C++ 后端基本功来支撑一份 Agent 应用开发方向的简历，它不是、也不需要是一个生产级 IM 系统。

这份路线图是课程顺序、范围、状态和交接的唯一事实来源。课程文件以“原理、调用链、设计、排障和面试表达”为主，代码只保留定位实现所需的关键片段。Agent 必须基于当前真实代码交付，并在每个任务中同时提交实现说明和验收证据。

## 工作区地图

- 后端总目录：`/home/deadnumb/C++ChatProject`
- 现有的 C++ HTTP 服务：`/home/deadnumb/C++ChatProject/apps/gate-server`
- VerifyServer：`/home/deadnumb/C++ChatProject/apps/verify-server`
- Windows Qt 客户端：`D:\QTChat\QTChat`

现有的 C++ 目录名叫 `ChatServer`，但它目前的职责其实是 GateServer：HTTP、注册、调用 VerifyServer 的 gRPC、Redis 和 MySQL。真正的 TCP ChatServer 目前还不存在。在教学材料中不要让目录名混淆这两者的职责。

后端两个仓库和 Qt 仓库里都有未提交的工作，必须保留。绝不允许丢弃或覆盖无关改动。

## 9 月 1 日的完成边界

当两个 Qt 客户端能做到以下几点时，demo 即为完成：

1. 注册和登录；
2. 向同一个 ChatServer 建立带认证的 TCP 连接；
3. 互相搜索到对方；
4. 发送并同意 Friend Request；
5. 重新加载并显示 Friend Contacts；
6. 打开双方共享的 Private Conversation；
7. 实时交换已持久化的文本消息；
8. 界面是干净、简洁的双栏布局。

## 明确延后的内容

- 完整送达 ACK、严格至少一次投递和复杂重试
- 客户端 SQLite 增量缓存
- StatusServer 与动态选择 ChatServer
- 多个 ChatServer 实例与跨服务器转发
- 心跳、强制下线与完整的重连策略
- 群聊、文件、图片、语音、已读回执、撤回、拉黑、标签、联系人分组
- 生产级安全、TLS、可观测性以及任何高可用声明

本期保留服务端消息落库，以及用户重新登录后的会话列表和历史消息分页同步；这属于历史恢复，不等同于客户端送达 ACK 或完整的至少一次投递。

ADR 中描述了一些 demo 之后的目标行为（包括至少一次投递）。课程可以提前准备兼容的 ID 或表结构字段，但绝不能把延后的行为拉进 9 月 1 日的关键路径。

## Demo 固定架构

1. Qt 通过 HTTP 与 GateServer 交互：获取验证码、注册、登录。
2. VerifyServer 通过 gRPC 发送并存储验证码（存储在 Redis）。
3. GateServer 校验凭证，把一个短时效登录 token 写入 Redis，并返回配置好的 ChatServer 地址和端口。
4. Qt 通过 TCP 直连这唯一的 ChatServer。
5. ChatServer 在 Redis 中校验 token，并把 User 绑定到活跃 Session。
6. ChatServer 处理用户搜索、Friend Request、Friend Contact、Private Conversation 和 Text Message，数据以 MySQL 为底。

## 课程必须遵守的决策

- [ADR 0001](../../adr/0001-use-teaching-compatible-tcp-framing.md)：2 字节消息 ID、2 字节 UTF-8 字节长度、JSON 正文、大端序，文本上限 8 KiB。
- [ADR 0002](../../adr/0002-defer-status-server-behind-login-contract.md)：当前固定使用单个 ChatServer；保留 `uid`、`host`、`port`、`token` 的登录契约。
- [ADR 0003](../../adr/0003-use-at-least-once-message-delivery.md)：向发送方返回成功前先持久化消息，使用稳定的消息 ID；本期实现历史同步，完整 ACK、重试和严格至少一次语义延后。
- [ADR 0004](../../adr/0004-store-two-friend-contacts-per-friendship.md)：Friend Request 按有向记录存储；每次 Friendship 在事务中创建两条 Friend Contact。
- [ADR 0005](../../adr/0005-create-private-conversation-with-friendship.md)：Private Conversation 随 Friendship 一起创建，但直到第一条 Text Message 之后才在「最近会话」中显示。

统一使用 [CONTEXT.md](../../../CONTEXT.md) 中的标准术语。

## 01 课关闭记录（2026-08-28）

第 01 课已满足关闭条件：真实邮箱在关闭 Clash TUN 后收到验证码，Qt 完成注册，MySQL 查到新的 User；注册后的邮箱再次获取验证码返回 `1007`，且没有重新写入或覆盖验证码。

本次验收同时记录了一个环境阻塞：Clash TUN 的 Fake-IP 让 `smtp.qq.com` 的直接 TLS 连接失败。该问题已通过关闭 TUN 验证为网络路径问题，不把它误归因于注册业务或 Outlook 投递过滤。

## 课程工作流

状态刻意保持简单：

- `draft`：仅有简版，还不能照着做。
- `ready`：已扩写成完整的实操教程。
- `closed`：学习者已运行并验证通过。

接续工作的 Agent 必须：

1. 阅读本路线图、当前课程、术语表和相关 ADR；
2. 检查当前代码和构建的真实状态，而不是相信旧的代码片段；
3. 只在 `current_lesson` 的前置阻塞课程都已 `closed` 时展开它；
4. 把课程内容限定在其可观察结果之内；
5. 写完教程后将其设为 `ready`；
6. 等待学习者运行并报告结果；
7. 只有在看到验收证据后才标记 `closed`，并推进 `current_lesson`。

不要提前为后续课程预生成详细代码。它们的 API 必须跟随前面课程产出的实际实现。

## 教程的必备结构

扩写后的课程可以调整标题，但必须包含：

1. 完成后学习者会看到什么
2. 动手改代码前需要理解的概念
3. 当前调用链与目标调用链
4. 开工前的检查项和命令
5. 有序的小步实现步骤
6. 基于当前真实仓库的、可运行的准确代码
7. 关键行的讲解，以及所有权/生命周期说明
8. 所有涉及进程的构建与启动命令
9. 预期的请求、响应、界面行为和日志
10. 常见错误及其具体排查步骤
11. 端到端验收清单
12. 用学习者自己的话讲出来的面试版讲解
13. 简短的自查问题

代码必须对新手可读，并且刻意保持克制。不要为了显得高级而引入抽象。绝不允许把 XOR、明文传输或本地 demo token 描述成生产级安全方案。

## 课程计划

| 课程 | 状态 | 小时 | 前置阻塞 | 可观察结果 |
| --- | --- | ---: | --- | --- |
| [01 注册基线](lessons/01-registration-baseline.md) | closed | 3 | 无 | Qt 完成验证码注册，MySQL 中出现该 User |
| [02 HTTP 登录交接](lessons/02-http-login-handoff.md) | ready | 3 | 01 | Qt 收到 `uid`、ChatServer 地址和 Redis 支撑的 token |
| [03 TCP 消息循环](lessons/03-tcp-message-loop.md) | draft | 4 | 02 | Qt 与 ChatServer 可靠交换带帧的 JSON |
| [04 聊天认证与主窗口](lessons/04-chat-auth-and-window.md) | draft | 3 | 03 | TCP token 认证成功，Qt 进入聊天窗口 |
| [05 用户搜索](lessons/05-user-search.md) | draft | 2 | 04 | 一个 User 能搜索并看到另一个 User |
| [06 Friend Request](lessons/06-friend-request.md) | draft | 3 | 05 | 已持久化的 Friend Request 送达在线接收方，重新登录后仍能加载 |
| [07 同意建立 Friendship](lessons/07-accept-friendship.md) | draft | 4 | 06 | 一个事务创建 Friendship、两条 Friend Contact 和 Private Conversation |
| [08 好友联系人列表](lessons/08-friend-contact-list.md) | draft | 2 | 07 | 双方重新加载 Friend Contacts 并打开空的 Private Conversation |
| [09 在线 Private Conversation](lessons/09-online-private-conversation.md) | draft | 4 | 08 | Text Message 持久化并实时出现在双端；会话进入「最近会话」 |
| [10 Demo 加固](lessons/10-demo-hardening.md) | draft | 2 | 09 | 可重复的双客户端演示，错误信息可理解，附带面试讲稿 |

原始教学计划约 30 个专注小时；本轮按 10 个专注小时执行，编码时间由 Agent 辅助完成，学习时间集中在调用链、核心原理、设计取舍、故障定位和面试表达。

## 课程级验收

- 每节课都按依赖顺序达到 `closed`。
- 每个实施任务都必须同时具备功能验收、失败验收、生命周期说明和面试版讲解。
- 存在一套干净的本地启动流程。
- 最终演示可以用两个账号反复进行，两次运行之间不需要改任何源码。
- 失败信息能区分出是 HTTP、TCP、Redis、MySQL 还是 token 校验出了问题。
- 学习者不看代码也能讲清楚 HTTP 到 TCP 的交接、TCP 帧协议、Session 绑定、Friendship 事务和实时消息路径。

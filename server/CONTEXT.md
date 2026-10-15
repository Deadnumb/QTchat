# 聊天上下文（统一术语表）

本文档定义了项目私聊流程所需的小型社交与消息模型的统一术语。正文、ADR、课程材料中必须使用这里的标准术语。

## 语言

**User（用户）**：
完成注册的人，可以登录、建立好友关系并参与私聊会话。
_避免使用_：Account、member

**Friend Request（好友请求）**：
一个 User 向另一个 User 发出的、等待处理的建立好友关系的请求。
_避免使用_：Add request、application

**Friendship（好友关系）**：
两个 User 之间已批准建立的对称关系，使他们可以开启一段私聊会话。
_避免使用_：Contact、friend record

**Friend Contact（好友联系人）**：
在一次 Friendship 中，某个 User 对另一个 User 的个人视角记录，包括仅属于该视角的私备注名。一次 Friendship 包含两条 Friend Contact 记录。
_避免使用_：Friendship、friend pair

**Private Conversation（私聊会话）**：
随 Friendship 建立的一对一会话，参与者仅有这两个 User。
_避免使用_：Chat room、thread

**Recent Conversation（最近会话）**：
至少包含一条 Text Message 的 Private Conversation，因此有资格出现在某个 User 的最近会话列表中。
_避免使用_：Empty conversation、contact

**Text Message（文本消息）**：
Private Conversation 的一方参与者发送给另一方的不可变文本。
_避免使用_：Chat data、packet

**Offline Message（离线消息）**：
发送时接收方不在线，因而等待后续投递的 Text Message。
_避免使用_：Cached message、failed message

**Delivered Message（已送达消息）**：
接收方客户端已确认收到的 Text Message。送达不代表接收方已读。
_避免使用_：Read message、completed message

## 教学参考与注册实现基线

本项目的注册、验证码和 GateServer/VerifyServer 教学主要参考：
[用户指定的 C++ 聊天项目教学博客](https://www.yuque.com/lianlianfengchen-cvvh2/dz8xhn/ig1no1iid1y1hmet)。

博客作为实现参考；课程顺序、当前行为和最终判断仍以本项目的 ROADMAP、ADR 和实际源码为准。

注册查重由 MySQL 存储过程 `reg_user` 完成：

- 用户名或邮箱已存在时返回 `0`。
- 注册成功时插入 User 并返回新 `uid`。
- DAO 或数据库异常返回 `-1`。
- `MySQLDao::RegUser` 负责调用存储过程并读取 `@result`；`LogicSystem` 中的 `uid == 0 || uid == -1` 不是第二次查重，而是解释结果并转换为 GateServer 错误。
- `RegUser` 是“查重并注册”的操作，不是纯查询，不能在发送验证码前调用，否则新邮箱可能被提前注册。

如果产品要求已注册邮箱不发送验证码，应增加独立的只读邮箱存在性查询，并在调用 VerifyServer 前执行；最终注册时仍保留 `reg_user` 作为数据一致性防线。

## Issue 1 验收与故障沉淀（2026-08-28）

Issue 1 已完成真实验收：关闭 Clash TUN 后，Qt 能收到真实验证码并完成注册，MySQL 中出现新的 User；注册后的邮箱再次请求验证码返回 `1007`，没有覆盖 Redis 验证码。

本次邮件阻塞不是注册查重。目标邮箱在数据库中不存在，VerifyServer 已生成验证码，但 SMTP 465 报 `connection reset by peer`。进一步检查发现 Clash TUN 将 `smtp.qq.com` 解析到了 `198.18.*` Fake-IP，Go 的直接 TLS 连接无法完成握手；关闭 TUN 后 SMTP 日志出现发送成功，邮箱实际收到验证码。

当前异步发信的成功语义必须说准确：VerifyServer 先写 Redis（TTL 180 秒），再启动 `SendMail` goroutine 并立即返回；HTTP/gRPC 成功不等于 SMTP 已接受，更不等于邮箱最终投递。生产化应补充 outbox 或队列、重试、状态和告警。

完整课程和面试故事见：
[第 01 课：端到端跑通注册流程](docs/learning/archive/private-chat-demo-legacy/lessons/01-registration-baseline.md)；
[Issue 1 注册与验证码投递面试故事](docs/learning/archive/private-chat-demo-legacy/interview-story-01-registration.md)。

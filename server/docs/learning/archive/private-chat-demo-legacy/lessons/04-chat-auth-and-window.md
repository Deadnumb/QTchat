---
status: draft
estimated_hours: 3
blocked_by: 03-tcp-message-loop
---

# 第 04 课：TCP Session 认证并进入聊天窗口

## 学习成果

HTTP 登录之后，Qt 连接 ChatServer，提交 User ID 和 token，收到认证结果，并打开简洁的双栏聊天窗口。

## 教程必须覆盖

- 把 Qt 的登录响应接到它的 TCP 管理器。
- 新增聊天登录的请求和响应消息 ID。
- 直接在 Redis 中校验 token，并把认证通过的 User 绑定到 Session。
- 拒绝未认证 Session 发来的业务消息。
- 保证 Session 生命周期和 User 到 Session 的查找是安全的。
- 用标准 Qt 布局和 QSS 搭建最小的主聊天界面。

## 验收清单

- [ ] 有效 token 能打开聊天窗口。
- [ ] 无效或过期 token 停留在登录页并显示错误。
- [ ] 认证之后 ChatServer 能通过 User ID 找到活跃 Session。
- [ ] 断开连接能清除过期的 Session 归属。

## 功能实现与接口质量

本课链路是 Qt 登录交接数据 → TCP 连接 → token 校验 → Session 绑定 → 主窗口。认证函数返回包含状态和 User ID 的结果对象；不要用 `bool CheckAuth(token, int& uid)`。认证失败、token 过期、连接断开必须是可观察的不同结果。

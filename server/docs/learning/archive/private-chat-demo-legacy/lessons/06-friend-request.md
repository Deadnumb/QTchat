---
status: draft
estimated_hours: 3
blocked_by: 05-user-search
---

# 第 06 课：持久化并推送 Friend Request

## 学习成果

User A 向 User B 发送 Friend Request；请求被持久化，在线的 B 立即看到，B 后续重新登录时也能加载到。

## 教程必须覆盖

- 定义有向的 Friend Request 表结构和状态值。
- 拒绝自己加自己、已有 Friendship、重复的未处理请求。
- 通过 MySQL 事务边界完成插入。
- B 在线时，经活跃的 User 到 Session 映射推送给 B。
- 在 B 认证后的初始化响应中包含未处理请求。
- 渲染最小的请求列表，带同意和拒绝按钮。

## 验收清单

- [ ] A 能向 B 提交一条未处理请求。
- [ ] 在线的 B 无需刷新即可看到。
- [ ] B 在做决定前断开重连，仍能再次看到。
- [ ] 重复请求和自己加自己的请求返回可理解的响应。
- [ ] 同意之前不存在 Friendship。

## 功能实现与接口质量

本课先定义请求状态流：创建请求 → 通知接收方 → 持久化 → 重连恢复。提交函数返回 `FriendRequestResult`，其中包含状态和请求 ID；不要用 `bool + int& request_id`。重复请求、自己请求自己和数据库失败必须有独立结果。

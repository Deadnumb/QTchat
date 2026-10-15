---
status: draft
estimated_hours: 4
blocked_by: 06-friend-request
---

# 第 07 课：用事务同意建立 Friendship

## 学习成果

User B 同意 A 的请求，一个 MySQL 事务更新请求状态、创建两条 Friend Contact、创建两人唯一的 Private Conversation。

## 教程必须覆盖

- 加锁并校验这条未处理的 Friend Request。
- 原子地插入两条有向 Friend Contact。
- 用有序 User ID 和唯一约束创建一个会话。
- 任何一步失败都回滚所有改动。
- 把新 Friend Contact 和会话标识返回给 B。
- 通知在线的 A：Friendship 已建立。
- 解释为什么数据库提前创建会话，而 UI 在出现消息之前不让它进入「最近会话」。

## 验收清单

- [ ] 同意后双方各获得一条 Friend Contact。
- [ ] 这对用户恰好存在一个 Private Conversation。
- [ ] 重复同意要么幂等，要么返回明确的终态结果。
- [ ] 强制制造失败后不会残留单边的 Friend Contact。
- [ ] 不会自动插入系统 Text Message。

## 功能实现与接口质量

本课的核心功能是一次事务内完成“同意请求 → 建立双方联系人 → 获取或创建会话”。事务函数返回 `AcceptFriendResult`，不要用多个输出参数分别返回联系人和会话 ID。失败时由事务回滚，调用方只依据结果状态继续流程。

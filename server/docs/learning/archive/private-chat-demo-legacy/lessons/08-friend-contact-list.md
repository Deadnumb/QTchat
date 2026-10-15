---
status: draft
estimated_hours: 2
blocked_by: 07-accept-friendship
---

# 第 08 课：加载 Friend Contacts 并打开 Private Conversation

## 学习成果

两个 User 重启后都能加载已持久化的 Friend Contacts，互相选中对方，打开共享的空 Private Conversation，且不会把它加进「最近会话」。

## 教程必须覆盖

- 从每个 User 的有向视角查询 Friend Contacts。
- 带上会话标识，但不重复 Friendship 数据。
- 用服务端状态填充 Qt 的好友联系人列表。
- Friend Contacts 与 Recent Conversations 保持分离。
- 选中联系人并初始化右侧会话面板。

## 验收清单

- [ ] 重启后双端都能显示对方。
- [ ] 任意一端选中对方，解析出的会话标识一致。
- [ ] 空会话不出现在「最近会话」里。
- [ ] 不需要任何硬编码的用户或会话数据。

## 功能实现与接口质量

本课按“服务端返回联系人数据 → 客户端映射模型 → UI 展示 → 选中会话”实现。列表查询返回值类型集合；会话 ID 应放在明确的联系人模型中，不通过 `int& conversation_id` 或 UI 全局变量传递。

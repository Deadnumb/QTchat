# 随 Friendship 一起创建 Private Conversation

同意一条 Friend Request 时，在同一个 MySQL 事务中创建两条 Friend Contact 和唯一的 Private Conversation，避免并发场景下首条消息才创建会话的竞争问题，也让后续消息拥有稳定的会话 ID。Qt 的最近会话列表在该会话拥有第一条 Text Message 之前保持为空，因此后端提前创建会话不会在界面上产生空条目。

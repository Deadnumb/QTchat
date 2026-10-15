# 每次 Friendship 存储两条 Friend Contact

好友请求按有向记录存储；同意一条请求时，在事务中创建两条有向 Friend Contact 记录，分别代表两个 User 各自对这次对称 Friendship 的视角。这与教学项目保持一致，使好友列表查询简单直接，并为每个用户独立的备注名留出空间；Private Conversation 仍然是单条共享记录，以有序对 `(min_uid, max_uid)` 标识并带唯一约束。

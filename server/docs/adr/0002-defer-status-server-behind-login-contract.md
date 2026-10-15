# 将 StatusServer 延后到登录契约之后

首个私聊版本只使用一个配置好的 ChatServer：GateServer 向 Redis 写入短时效 token，并返回 `uid`、`host`、`port`、`token`；ChatServer 直接从 Redis 校验该 token。独立的 StatusServer 延后到需要多个 ChatServer 实例时再做，但对客户端的登录契约保持不变，以便将来把服务选择逻辑从 GateServer 中移出去时，不需要改动 Qt 的登录流程。

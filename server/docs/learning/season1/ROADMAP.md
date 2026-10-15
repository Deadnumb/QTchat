# 第一季：一周照教程完成主链路

## 目标

教程原本按 30 个小节展开。本路线把它压成 7 个可以每天验收的学习日：

- 每天只推进一条纵向链路；
- 先照着教程顺序复制，再回头解释代码；
- 每天留下可观察证据；
- 当前仓库和教程不一致的地方，以当前仓库为准。

教程入口：[C++ 聊天项目第一季](https://www.yuque.com/lianlianfengchen-cvvh2/dz8xhn/lb4b8aam6tgcioif)。

## 当前进度

| 能力 | 当前状态 | 证据位置 |
| --- | --- | --- |
| GateServer HTTP | 已有 Beast HTTP、/get_verifycode、/user_register、/user_login；注册/登录已做本地接口验收 | apps/gate-server/src/LogicSystem.cpp |
| VerifyServer | Go gRPC 服务已在单仓库中 | apps/verify-server |
| 验证码 | Redis 写入和 SMTP 路径已存在 | VerifyServer 代码与运行日志 |
| 注册入库 | MySQL reg_user 存储过程已存在，注册接口已返回 uid | db/mysql/init/01_init.sql、LogicSystem.cpp |
| HTTP 登录 | CheckLogin、/user_login、Redis token 已实现；正确/错误密码和 TTL 已验收 | apps/gate-server/src/LogicSystem.cpp、MySQLDao.cpp |
| Qt 登录交接 | LoginDialog 已调用 /user_login 并保存四个字段；手工点击待验收 | Windows Qt 工程 logindialog.cpp |
| TCP ChatServer | 尚未加入当前后端 | Day3、Day6 |
| 好友和消息 | 尚未形成端到端闭环 | Day5、Day6 |

## 当前停靠点

Day1 的后端链路已经完成，Day2 的 Qt 登录交接代码已经编译。打开 Qt 手工确认登录成功和错误提示后，直接进入 Day3 的 TCP 认证，不再继续扩展 HTTP 字段。

## 七天安排

| 天 | 对照教程 | 当天主线 | 结束时必须看到 |
| --- | --- | --- | --- |
| Day1 | 1～11、14 | 复盘注册基线，理解 GateServer 登录查询、token 和 /user_login | 已看到正确登录、错误密码 1009、Redis TTL；代码实现完成 |
| Day2 | 12～14 | Qt 登录、注册、重置页面接通 HTTP | 代码已编译；打开 Qt 后手工确认成功/失败提示和 LoginSession |
| Day3 | 15～17 | Qt TCP 客户端、ChatServer 接收和基础帧 | 建立 TCP 连接并完成一条回显或认证消息 |
| Day4 | 18～24 | 主窗口、搜索框、聊天列表、气泡和侧栏 | 登录后进入聊天窗口并切换列表 |
| Day5 | 25～26 | 好友申请界面、联系人列表 | 展示待处理申请和联系人 |
| Day6 | 27～29 | 好友查询、申请确认、私聊通信 | 两个账号成为好友并交换文本 |
| Day7 | 30 | 全链路回归、故障排查、面试复述 | 一周验收表的 P0 项全部有证据 |

## 教程章节和当前仓库的转换

| 教程里的写法 | 当前仓库的落点 |
| --- | --- |
| Node.js 邮箱服务 | 使用 apps/verify-server 的 Go 服务，不再复制 Node 服务 |
| 教程中的 ChatServer 目录 | 当前 apps/gate-server 实际是 HTTP GateServer；TCP 服务单独建目录 |
| Visual Studio / Windows Boost 配置 | 后端统一在 WSL 使用 GCC、CMake、Ninja；Windows 只编 Qt |
| 固定密码和固定 token | 使用本地演示账号和运行时生成的 token |
| StatusServer / 多服务发现 | 第一周固定一个 ChatServer 地址，先完成单机闭环 |

## 每天的学习闭环

~~~text
看教程标题
  → 对照当前文件
  → 复制当天代码块
  → 编译
  → 启动依赖
  → 成功用例
  → 失败用例
  → 用自己的话解释一遍
~~~

遇到 Docker 拉镜像失败时，先判断 MySQL、Redis 和本地二进制能否使用。网络阻塞单独记录，不把它和 C++ 代码失败混在一起。

## 本周明确不做

- 不把真实秘钥提交到 Git；
- 不把明文密码或完整 token 打进日志；
- 不实现 TLS、群聊、文件、复杂重试和高可用；
- 不把 HTTP 登录成功写成 TCP 已连接；
- 不把静态编译成功写成端到端完成。

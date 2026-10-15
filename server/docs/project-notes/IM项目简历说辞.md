# IM项目简历说辞

---

## 项目概述

独立开发，基于 Proactor 模型的高性能IM后端服务器，O3优化后，单机 QPS 达6万，平均延迟 <1ms。

**对应描述：** 本项目从零独立设计并实现了一个完整的即时通讯后端系统。服务端采用 C++ 开发，基于 Boost.Asio 的 Proactor 异步 I/O 模型构建，通过 CMake 管理构建，Release 模式下启用 -O3 最高级别编译器优化。项目涵盖了网络层、业务逻辑层、数据访问层以及微服务间的 RPC 通信，配合 Qt 客户端和 Node.js 验证码微服务，形成了一套完整的 IM 系统闭环。

性能验证方面，使用 wrk 压测工具在本地环境进行基准测试，参数为 3 个线程、100 个并发连接，对纯计算型端点（GET /get_test，直接写回响应字符串，不涉及外部 I/O）持续压测 30 秒，测得 QPS 约 6 万，平均延迟低于 1ms。这个测试主要验证的是 HTTP 层（Boost.Beast 解析 + Asio 异步 I/O）的吞吐上限，属于 CPU-bound 场景，不包含 gRPC / Redis / MySQL 的外部调用开销。对于涉及下游服务调用的端点（如 /get_verifycode 需同步等待 gRPC 响应），吞吐会受下游延迟约束，属于另一类性能剖面。目前测试方法的局限性在于：wrk 与服务端跑在同一台机器上，存在资源竞争；仅测了平均延迟，缺少 p99 / p999 等长尾指标。如果要更严谨地评估，下一步计划用 wrk2 做恒定速率压测来确定真正的饱和拐点，用 perf 工具做 CPU profiling 定位热点函数，并将 wrk 部署到独立机器上消除本地干扰——这些改进方向在面试中主动提起，反而能体现对性能测试方法论的理解。

---

## 基于 Asio 实现 Proactor 模式，主线程 accept，轮询分发给子线程避免单点瓶颈

**对应描述：** 这里需要先厘清一个容易混淆的点，也是面试中能体现深度的细节：Boost.Asio 对外暴露的编程接口是 Proactor 风格的——调用 `async_read` / `async_write` / `async_accept` 时不是"等数据就绪后通知我，我来读"，而是"帮我完成整个 I/O 操作，完成后回调我"。但从底层实现来看，这套 Proactor 接口在 Linux 下并非真正的异步 I/O，而是通过 epoll（Reactor 模式）模拟出来的：epoll_wait 监听到 fd 可读/可写事件后，Asio 内部在用户态完成实际的 read/write 系统调用，再触发用户注册的完成回调。在 Windows 上 Asio 则直接对接 IOCP，是真正的操作系统级 Proactor。理解这个差异有助于在 Linux 环境下正确评估性能上限——epoll 模式下的 I/O 读写仍然发生在用户态线程中，内核态/用户态的数据拷贝开销并未消除，真正的优势在于事件通知的高效（O(1) 而非 select/poll 的 O(n)）和编程模型的简洁。

在具体架构设计上，主线程运行一个独立的 `io_context`，专门负责 `tcp::acceptor` 的 `async_accept` 操作。当新连接到达时，accept 回调中从 `AsioIOServicePool` 通过轮询算法（`_nextIOService = (_nextIOService + 1) % size`）获取一个工作线程的 `io_context`，并在该 `io_context` 上创建 `HttpConnection` 对象。这样每个新连接被分配到一个工作线程后，该连接后续的所有异步 I/O（`async_read`、`async_write`、deadline 定时器）都绑定在同一个 `io_context` 上执行，天然实现了连接粒度的负载均衡，避免了单个 I/O 线程成为多核场景下的性能瓶颈。

`AsioIOServicePool` 内部维护了 `std::thread::hardware_concurrency() * 2` 个 `io_context` 实例，每个实例都持有一个 `executor_work_guard` 防止 `run()` 提前返回，并运行在独立的 `std::thread` 中。每个工作线程内部是一个 `epoll` 事件循环——`io_context.run()` 在 Linux 下最终调用 `epoll_wait` 等待所辖连接上的 I/O 事件，然后分派到对应的完成回调。这种设计充分利用了多核 CPU 的并行能力，同时又保证了同一连接的所有回调都在同一线程内串行执行，从根本上避免了数据竞争，不需要在业务层额外加锁。

---

## 使用 gRPC 完成微服务间调用，随机生成验证码返回

**对应描述：** 在架构层面上，我将验证码的生成与发送逻辑从主网关服务中解耦为独立的微服务，两者通过 gRPC 进行通信。这样做的考虑是：验证码生成涉及邮件发送等 I/O 密集型操作，独立部署后可以单独扩缩容，不影响主服务的吞吐能力。

在 `.proto` 文件中定义了 `VerifyService` 服务，包含一个一元 RPC `GetVerifyCode(GetVerifyReq) returns (GetVerifyRsp)`，请求携带邮箱地址，响应返回错误码、邮箱和生成的验证码字符串。GateServer 收到客户端的获取验证码请求后，通过 `VerifyGrpcClient`（单例模式）获取 gRPC stub，发起同步调用获取验证码并返回给客户端。

VerifyServer 微服务独立运行在 50051 端口，收到 gRPC 请求后使用 `uuid` 库随机生成唯一验证码，然后通过 Nodemailer 对接 QQ 邮箱 SMTP 服务器（smtp.qq.com:465，SSL 加密），将验证码以邮件形式发送到用户邮箱，并将验证码字符串通过 gRPC 响应返回。同时，验证码写入 Redis（key 前缀 `verify_code_` + 邮箱），设置 3 分钟过期，供后续注册流程校验使用。

---

## 手写 Asio 连接池、gRPC 连接池、Redis 连接池、MySQL 连接池

**对应描述：** 连接池是本项目性能优化的核心手段之一，我手写了三套不同场景的连接池，针对各自的使用特点做了差异化设计。

**Asio 连接池**（`AsioIOServicePool`）：严格来说这是一个 `io_context` 线程池，而非传统意义上的连接池。构造时创建 `硬件并发数 × 2` 个 `io_context`，每个绑定一个 `executor_work_guard` 保活，运行在独立线程上。对外暴露 `GetIOService()` 接口，以轮询方式返回下一个 `io_context`，将新连接均匀分配到各工作线程。停止时按序清空 work guard、停止 io_context、join 所有线程，保证优雅退出。

**gRPC 连接池**（`RPConPool`）：维护一个 `std::queue<std::unique_ptr<VerifyService::Stub>>`，由 `std::mutex` 和 `std::condition_variable` 保护。构造时预创建 `poolSize`（5）个 gRPC Channel 和 Stub，使用 `grpc::InsecureChannelCredentials`（内网环境下可接受）。`getConnection()` 使用 `cond_.wait(lock, predicate)` 阻塞等待直到有可用连接或被通知停止，返回时通过 `std::move` 转移 `unique_ptr` 所有权。`returnConnection()` 将使用完毕的 Stub 移回队列并通知等待者。通过 `std::atomic<bool> b_stop_` 实现安全的连接池关闭。

**MySQL 连接池**（`MySQLPool`）：除基本的互斥锁 + 条件变量获取/归还机制外，还实现了一个心跳保活线程。该线程定期遍历池中所有连接，检查 `last_oper_time` 是否超过 5 秒，若超过则执行 `SELECT 1` 心跳查询。如果心跳抛出异常（说明连接已断开），则通过 MySQL Connector/C++ 驱动重新创建连接并替换。心跳检查中使用了 RAII 风格的 `Defer` 工具类（析构时自动归还连接到队列），确保即使心跳异常也不会导致连接泄漏。连接通过 `unique_ptr` 管理生命周期，`SQLConnection` 包装器额外记录了最后操作时间戳。

**Redis 连接池**：基于 redis++ 库内置的连接池能力，配置连接池大小为 10，等待超时 100ms，套接字超时 200ms。在 RedisManager 的单例构造函数中完成初始化，对外通过 `operator->()` 直接暴露 redis++ 的接口，上层调用简洁自然。

---

## 应用现代 C++ 特性（智能指针、右值引用、Lambda、CRTP）保障内存安全与性能

**对应描述：** 在整个项目中我刻意使用了现代 C++ 的特性来替代原始指针和手动内存管理，从语言层面杜绝内存泄漏和悬挂指针的风险。

**智能指针：** 所有对象的生命周期管理全部依赖 `shared_ptr` 和 `unique_ptr`。`CServer` 和 `HttpConnection` 继承自 `enable_shared_from_this`，在异步回调 lambda 中通过 `shared_from_this()` 捕获自身，保证在异步操作进行期间对象不会被提前析构——这是 Asio 异步编程中最容易出现 use-after-free 的地方，我用这个机制彻底规避了。`unique_ptr` 用于连接池中连接对象的独占所有权管理，配合 `std::move` 在队列出入时零开销地转移所有权，语义清晰且无法被误拷贝。

**右值引用与移动语义：** 连接池的获取和归还操作大量使用 `std::move`，将 `unique_ptr` 从队列移出交给调用方，使用完毕再移回队列。整个过程不涉及任何深拷贝，`unique_ptr` 本身的移动就是指针赋值，开销极小。`RPConPool::getConnection()` 和 `MySQLPool::returnConnection()` 中都能看到这种模式。

**Lambda 表达式：** 项目中最核心的回调机制全部由 lambda 实现。Asio 的 `async_accept`、`async_read`、`async_write`、`deadline_` 定时器均以 lambda 作为完成处理函数；`LogicSystem` 的路由表 `std::map<std::string, std::function<void(shared_ptr<HttpConnection>)>>` 的 value 全部是 lambda，在构造函数中注册时按值捕获智能指针，确保回调执行时连接对象有效。`MySQLPool` 的心跳线程和 `AsioIOServicePool` 的线程创建也使用 lambda 作为线程函数。

**CRTP（奇异递归模板模式）：** 项目中的 `Singleton<T>` 模板基类是整个依赖注入的基础设施。`AsioIOServicePool`、`LogicSystem`、`VerifyGrpcClient`、`RedisManager`、`MySQLManager` 均通过 `public Singleton<Derived>` 继承获得单例能力。Singleton 内部使用 `std::call_once` + `std::once_flag` 保证多线程环境下的初始化安全，利用 CRTP 的静态多态特性使 `get_instance()` 能直接返回 `shared_ptr<T>` 而非基类指针，避免了 `dynamic_cast` 的运行时开销。

**其他特性：** 项目中还使用了 `Defer` 工具类（析构时执行回调，类似 Go 的 defer）保证资源释放、`std::atomic<bool>` 实现无锁的状态标志、`= default / = delete` 明确控制特殊成员函数的生成、`auto` 类型推导简化复杂模板类型的声明。

---

## 基于 Qt 实现客户端界面（注册、验证码交互），完成与后端的数据对接

**对应描述：** 客户端采用 Qt6 Widgets 框架开发，通过 CMake 管理构建，界面由 `MainWindow` 作为容器，管理 `LoginDialog` 和 `RegisterDialog` 两个子窗口之间的切换。窗口采用无边框设计，通过 QSS 样式表统一视觉风格。

注册流程的设计充分体现了前后端协议对接的思路：用户在注册界面输入用户名、邮箱、密码等信息后，点击"获取验证码"按钮，客户端首先通过正则表达式校验邮箱格式合法性，然后构造 JSON 请求体 `{"email": "..."}` 通过 `HttpManager` 单例以 POST 方式异步发送到 GateServer 的 `/get_verifycode` 端点。`HttpManager` 封装了 `QNetworkAccessManager`，利用 Qt 的信号槽机制将网络响应异步回调到 `RegisterDialog`，后者维护了一个 `QMap<ReqId, std::function<void(const QJsonObject&)>>` 的回调映射表，根据请求 ID 将响应派发到对应的处理函数。

验证码获取成功后，提示用户查收邮件。用户填写验证码并确认注册时，客户端再次构造包含所有字段的 JSON 请求发送到 `/user_register` 端点，由服务端完成验证码校验和数据库写入。整个网络通信完全异步，不阻塞 UI 线程，用户体验流畅。错误提示通过 QSS 属性选择器（`[state="error"]` vs `[state="normal"]`）动态切换标签颜色，用 `repolish()` 工具函数强制重绘实现即时反馈。

---

## 采用微服务架构解耦部分功能，回调驱动的状态机维护代码

**对应描述：** 项目的整体架构体现了微服务解耦的思想：GateServer 作为 API 网关负责 HTTP 接入和业务路由，VerifyServer 作为独立的验证码微服务通过 gRPC 暴露接口，Redis 承担临时状态存储（验证码缓存），MySQL 负责用户数据的持久化，各组件通过明确的协议边界分离关注点。这种设计使得验证码服务的修改、升级和扩容不会波及主网关服务。

关于状态机设计，本项目虽然没有显式定义 enum+switch 的状态机类，但实际上 HTTP 连接的整个生命周期就是一个回调驱动的隐式状态机：`Start()`（读取请求）→ `HandleReq()`（路由分发与业务处理）→ `WriteResponse()`（写回响应）→ 根据 `keep_alive` 标志决定是回到 `Start()` 继续读取下一个请求，还是 `shutdown` 关闭连接。每个状态的转换都由 Asio 异步操作完成时的回调 lambda 驱动，而非阻塞等待。`HttpConnection` 还设置了 60 秒的 deadline 定时器，若业务处理超时则强制关闭 socket，这是一个典型的超时保护状态转换。

`LogicSystem` 的路由注册机制也是一种状态驱动的设计：通过 `RegGet` / `RegPost` 将 URL 路径映射到处理函数，请求到达时根据 HTTP 方法和 URL 查找对应的 handler 并回调执行。这种 map + lambda 回调的模式使得添加新接口只需一行注册代码，扩展性好且逻辑清晰。

另外，当前所有业务逻辑（JSON 解析、gRPC 调用、Redis 查询、MySQL 存储过程）均以同步方式直接运行在 `io_context` 工作线程的回调链上，没有引入独立的业务线程池。这样做的考虑是：当前项目为短连接场景（注册、登录、获取验证码），单次请求的业务处理耗时极短（gRPC 调用内网延迟在百微秒级，Redis/MySQL 查询在毫秒级），远小于网络 I/O 的耗时波动，将业务逻辑同步嵌入 io_context 线程不会构成瓶颈。引入业务线程池反而会带来请求跨线程调度开销、共享数据加锁复杂度、以及异步回调链的调试困难——在现阶段业务规模下得不偿失。之前 wrk 压测 `/get_test`（零外部 I/O 的纯计算端点）达到 6 万 QPS，验证了 HTTP 层的吞吐上限；而实际业务端点的瓶颈在 gRPC/Redis/MySQL 的下游延迟，不是 io_context 线程的 CPU 调度，所以同步模型在这里是合适的。

# Tiny RPC Framework

一个基于 **C++ + Muduo + Protobuf** 的轻量级 RPC 框架，实现高性能、低耦合的跨进程远程过程调用（RPC），支持像本地函数一样调用远端服务。

---

## 一、项目简介

Tiny RPC Framework 是一个从零实现的轻量级 RPC 框架，基于 Muduo 提供高性能 TCP 网络通信，使用 Google Protocol Buffers 作为接口描述语言（IDL）和序列化工具，通过自定义 RPC 协议和通用帧编解码机制，实现客户端与服务端之间的透明远程调用。

该项目适合：
- 学习 RPC 框架底层实现原理；
- 理解 Muduo 网络编程模型；
- 理解 Protobuf Service / Stub 的工作机制；
- 作为轻量级 RPC 通信组件嵌入 C++ 后端项目。

---

## 二、整体架构

### 1. 分层架构设计

```
┌────────────────────────┐
│        业务层          │  EchoService / 用户自定义 Service
└──────────▲─────────────┘
           │
┌──────────┴─────────────┐
│        RPC 层          │  RpcChannel / RpcCodec / RpcDispatcher / RpcController
└──────────▲─────────────┘
           │
┌──────────┴─────────────┐
│     网络抽象层         │  INetworkServer / RpcConnection / FrameCodec
└──────────▲─────────────┘
           │
┌──────────┴─────────────┐
│   Muduo 实现层         │  MuduoNetworkServer / MuduoRpcConnection
└────────────────────────┘
```

- **网络层**：基于 Muduo 实现 TCP 通信
- **帧编解码层**：解决 TCP 粘包、半包问题
- **RPC 协议层**：封装 Service/Method、请求响应、错误码
- **业务层**：开发者只需专注 Protobuf + Service 实现

---

### 2. RPC 数据帧格式

**完整 TCP 帧格式：**

```
[ total_len | meta_len | meta_bytes | body_bytes ]
   4字节        4字节
```

- `total_len`：整帧长度（大端序）
- `meta_len`：RPC 元信息长度
- `meta_bytes`：RpcMeta Protobuf 序列化数据
- `body_bytes`：请求或响应消息体数据

---

## 三、目录结构说明

```
tiny_rpc_framework/
├── include/
│   ├── net/                 # 通用网络帧封装
│   │   ├── frame_codec.h
│   │   └── network_server.h
│   ├── net_muduo/           # Muduo 网络适配层
│   │   ├── muduo_network_server.h
│   │   └── muduo_rpc_connection.h
│   └── rpc/                 # RPC 核心逻辑
│       ├── rpc_channel.h
│       ├── rpc_codec.h
│       ├── rpc_connection.h
│       ├── rpc_controller.h
│       ├── rpc_dispatcher.h
│       └── rpc_meta.pb.h
│
├── src/
│   ├── net_muduo/
│   │   └── muduo_network_server.cc
│   └── rpc/
│       ├── rpc_channel.cc
│       ├── rpc_codec.cc
│       ├── rpc_dispatcher.cc
│       └── rpc_meta.pb.cc
│
├── proto/
│   ├── echo.proto
│   └── rpc_meta.proto
│
├── examples/
│   └── echo/
│       ├── echo_server_impl.h
│       ├── echo_server_main.cc
│       └── echo_client_main.cc
│
├── CMakeLists.txt
└── README.md
```

---

## 四、核心模块说明

### 1. FrameCodec（粘包拆包）

- 使用长度前缀 `[uint32_t total_len]` 进行拆包
- 支持半包、粘包自动处理
- 对上层只暴露“完整帧”回调接口

---

### 2. RpcCodec（RPC 编解码）

- 负责 `RpcMeta` + Protobuf 消息的打包/解包
- 编码流程：
  ```text
  request/response → Protobuf序列化 → 拼接RpcMeta → 添加总长度
  ```
- 解码流程：
  ```text
  Frame → 解析meta_len → 解析RpcMeta → 剩余作为body
  ```

---

### 3. SimpleRpcChannel（客户端核心）

- 实现 `google::protobuf::RpcChannel`
- 管理请求 ID 与回调的映射关系
- 支持：
  - 多并发 RPC 调用
  - 异步回调
  - 请求自动匹配响应

---

### 4. RpcDispatcher（服务端分发器）

- 管理 `service_name → Service` 映射
- 动态查找 Method 并通过反射调用
- 自动封装响应并下发给客户端

---

### 5. Muduo 网络适配层

- `MuduoNetworkServer`：包装 Muduo TCP Server
- `MuduoRpcConnection`：对 `TcpConnection` 的轻量级封装
- 对上层完全屏蔽 Muduo 细节，保证网络解耦

---

## 五、工作时序（RPC 调用流程）

### RPC 请求–响应完整流程：

1. 客户端调用 Stub 接口（如 `Echo()`）
2. `RpcChannel`：
   - 构造 `RpcMeta`
   - 编码请求帧
   - 发送 TCP 数据
3. 服务端：
   - `FrameCodec` 拆帧
   - `RpcDispatcher` 反射调用业务 Service
4. 业务处理完成
5. `RpcDispatcher` 编码响应帧
6. 响应帧通过 TCP 回发客户端
7. 客户端解码 -> 触发回调 -> 用户获得结果

---

## 六、使用示例（Echo Demo）

### 1. 启动服务端

```bash
./echo_server
```

服务端会监听指定端口，等待客户端连接。

---

### 2. 启动客户端

```bash
./echo_client
```

客户端将向服务端发送 Echo 请求并接收返回结果。

---

### 3. 示例业务接口（echo.proto）

```proto
syntax = "proto3";
package demo;
option cc_generic_services = true;

message EchoRequest {
  string message = 1;
}

message EchoResponse {
  string message = 1;
}

service EchoService {
  rpc Echo(EchoRequest) returns (EchoResponse);
}
```

---

## 七、编译方式

### 1. 依赖

- Linux
- gcc / g++ (支持 C++17)
- Muduo
- Protobuf
- CMake ≥ 3.10

---

### 2. 编译步骤

```bash
mkdir build
cd build
cmake ..
make -j
```

生成：

- RPC 框架静态库：`libtiny_rpc.a`
- Echo 示例程序：`echo_server`, `echo_client`

---

## 八、项目特点

- ✅ 支持 Protobuf Service 自动分发
- ✅ 支持 TCP 粘包 / 半包处理
- ✅ 支持多并发请求 ID 映射
- ✅ 网络层与 RPC 层完全解耦
- ✅ 结构清晰，适合二次开发与学习

---

## 九、可扩展方向

- 服务注册与发现（Zookeeper / Etcd）
- 连接池与负载均衡
- 超时控制与熔断机制
- 协程化 RPC 调用
- HTTP / gRPC 协议适配

---

## 十、作者与说明

本项目为个人学习与实践 C++ RPC 框架的练习项目，适合作为：

- C++ 网络编程学习项目
- RPC 框架原理教学示例
- 后端分布式系统基础组件

欢迎二次开发与交流使用。



#pragma once
#include <functional>
#include <memory>
#include <string>

class RpcConnection; // 前向声明，协议层抽象连接

// 纯网络服务器接口（策略模式抽象）
class INetworkServer {
public:
    using MessageHandler = std::function<void(
        std::shared_ptr<RpcConnection>,      // 抽象连接
        const std::string&                   // 一条完整 frame（二进制）
    )>;

    virtual ~INetworkServer() = default;

    // 启动/停止服务器
    virtual void Start() = 0;
    virtual void Stop() = 0;

    // 注册“上层消息回调”，每收到一帧完整数据就回调一次
    virtual void SetMessageHandler(MessageHandler handler) = 0;
};

#pragma once
#include <memory>
#include <string>

class RpcConnection;


/*网络层（INetworkServer/MuduoNetworkServer）真正需要的能力只有一个：
  - 拆出完整 frame 后，交给“上层某个东西”去处理。

  抽象出MessageHandler，具体如何处理frame它不需要知道（面向抽象编程）
  所有“处理frame的具体实现”，都要继承MessageHandler
*/
class MessageHandler {
public:
    virtual ~MessageHandler() = default;

    virtual void HandleMessage(
        const std::shared_ptr<RpcConnection>& conn,
        const std::string& frame) = 0;
};

class INetworkServer {
public:
    virtual ~INetworkServer() = default;

    virtual void Run() = 0;
    virtual void Stop() = 0;

    // 注入上层处理器（RPC / HTTP / 其他）
    virtual void SetMessageHandler(
        std::shared_ptr<MessageHandler> handler) = 0;

    /*为什么不在这里持有一个handler？
      抽象基类——>"抽象能力"（接口），如果持有handler，就限制了具体类的实现（每个都被迫拥有handler）
      应该是“最小抽象”，给予具体类足够的实现空间，避免过度约束
    */
};

#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include "net_muduo/muduo_network_server.h"
#include "rpc/rpc_dispatcher.h"
#include "rpc/rpc_codec.h"
#include "echo_server_impl.h"

int main() {
    using namespace muduo;
    using namespace muduo::net;

    EventLoop loop;
    InetAddress listenAddr(12345);

    // 1. 协议层：RPC 分发器
    RpcDispatcher dispatcher;
    EchoServiceImpl echoService;
    dispatcher.RegisterService(&echoService);

    // 2. 网络层：选择 Muduo 实现的 INetworkServer
    MuduoNetworkServer server(&loop, listenAddr);

    // 3. 绑定“收到完整 frame 后的处理逻辑”
    server.SetMessageHandler(
        [&dispatcher](std::shared_ptr<RpcConnection> conn,
                      const std::string& frame)
        {
            rpc::RpcMeta meta;
            std::string payload;
            if (!RpcCodec::DecodeFrame(frame, &meta, &payload)) {
                // 解码失败，按需要记录日志或关闭连接
                return;
            }
            dispatcher.OnRpcMessage(conn.get(), meta, payload);
        });

    server.Start();
    loop.loop();
    return 0;
}

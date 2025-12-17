#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <memory>
#include "net_muduo/muduo_network_server.h"
#include "rpc/rpc_dispatcher.h"
#include "rpc/rpc_codec.h"
#include "rpc/rpc_server.h"
#include "rpc/rpc_server_factory.h"
#include "echo_server_impl.h"


int main() {
    using namespace muduo;
    using namespace muduo::net;

    std::unique_ptr<RpcServer> server = std::move(RpcServerFactory()
                                        .WithPort(12345)
                                        .WithIOThreads(4)
                                        .WithNetwork(NetworkType::Muduo)
                                        .Build());
    EchoServiceImpl echoService;
    server->RegisterService(&echoService);
    server->Run();

    return 0;
}

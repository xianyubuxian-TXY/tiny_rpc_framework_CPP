#include "rpc/rpc_server_factory.h"
#include "net_muduo/muduo_network_server.h"


RpcServerFactory& RpcServerFactory::WithPort(int port){
    port_=port;
    return *this;
}

RpcServerFactory& RpcServerFactory::WithNetwork(NetworkType type){
    net_type_=type;
    return *this;
}

RpcServerFactory& RpcServerFactory::WithIOThreads(int n){
    io_threads_=n;
    return *this;
}

std::unique_ptr<RpcServer> RpcServerFactory::Build() {
    std::unique_ptr<INetworkServer> network;

    switch (net_type_) {
    case NetworkType::Muduo:
        network = std::make_unique<MuduoNetworkServer>(port_, io_threads_);
        break;
    default:
        throw std::runtime_error("Unsupported network type");
    }

    return std::make_unique<RpcServer>(std::move(network));
}

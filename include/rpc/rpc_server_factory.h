#pragma once
#include<memory>
#include "rpc_server.h"


enum class NetworkType {
    Muduo,
    // Asio,
    // Epoll,
};

class RpcServerFactory {
public:
    RpcServerFactory()=default;

    RpcServerFactory& WithPort(int port);
    RpcServerFactory& WithNetwork(NetworkType type);
    RpcServerFactory& WithIOThreads(int n);

    std::unique_ptr<RpcServer> Build();

private:
    int port_ = 0;
    int io_threads_ = 1;
    NetworkType net_type_ = NetworkType::Muduo; //默认为Muduo库
};

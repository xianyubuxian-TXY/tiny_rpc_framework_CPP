#pragma once
#include "net/network_server.h"
#include "net/frame_codec.h"
#include "net_muduo/muduo_rpc_connection.h"

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Logging.h>
#include <boost/any.hpp>

class MuduoNetworkServer : public INetworkServer {
public:
    MuduoNetworkServer(muduo::net::EventLoop* loop,
                       const muduo::net::InetAddress& listenAddr);

    void Start() override;
    void Stop() override;
    void SetMessageHandler(MessageHandler handler) override;

private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn);
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buffer,
                   muduo::Timestamp);

private:
    muduo::net::TcpServer server_;
    MessageHandler handler_;
};

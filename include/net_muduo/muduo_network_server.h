#pragma once
#include "net/network_server.h"
#include "net_muduo/muduo_rpc_connection.h"
#include "net/frame_codec.h"
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Logging.h>
#include <boost/any.hpp>

struct ConnContext {
    std::string inbuf;
};


class MuduoNetworkServer : public INetworkServer {
public:
    MuduoNetworkServer(int port,int threadNum=4);

    void Run() override;
    void Stop() override;
    void SetMessageHandler(std::shared_ptr<MessageHandler> handler) override;

private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn);
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buffer,
                   muduo::Timestamp);

private:
    muduo::net::TcpServer server_;
    muduo::net::EventLoop loop_;
    FrameCodec frame_codec_;
    /*frame处理类：网络模块解析出frame后，通过这个进行处理即可——>由rpc_server注入*/
    /*网络模块只负责提取出frame，具体如何处理交给“上层注入的处理类/方法”*/
    std::shared_ptr<MessageHandler> handler_;
};
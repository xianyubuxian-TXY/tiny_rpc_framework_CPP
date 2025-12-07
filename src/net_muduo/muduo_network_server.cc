#include "net_muduo/muduo_network_server.h"

using namespace muduo;
using namespace muduo::net;
using namespace std::placeholders;

MuduoNetworkServer::MuduoNetworkServer(EventLoop* loop,
                                       const InetAddress& listenAddr)
    : server_(loop, listenAddr, "RpcServer")
{
    server_.setConnectionCallback(
        std::bind(&MuduoNetworkServer::onConnection, this, _1));

    server_.setMessageCallback(
        std::bind(&MuduoNetworkServer::onMessage, this, _1, _2, _3));
}

void MuduoNetworkServer::Start() {
    server_.start();
}

void MuduoNetworkServer::Stop() {
    // Muduo 通常通过 EventLoop 控制退出，这里可以留个空实现或调用 loop->quit()
}

void MuduoNetworkServer::SetMessageHandler(MessageHandler handler) {
    handler_ = std::move(handler);
}

void MuduoNetworkServer::onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        LOG_INFO << "New connection from " << conn->peerAddress().toIpPort();
        conn->setContext(std::string()); // 每个连接一个缓冲区
    } else {
        LOG_INFO << "Connection down from " << conn->peerAddress().toIpPort();
        conn->shutdown();
    }
}

void MuduoNetworkServer::onMessage(const TcpConnectionPtr& conn,
                                   Buffer* buffer,
                                   Timestamp)
{
    std::string& inbuf = *boost::any_cast<std::string>(conn->getMutableContext());
    inbuf.append(buffer->peek(), buffer->readableBytes());
    buffer->retrieveAll();

    // 用纯网络层的 FrameCodec 拆帧，拆出来的 frame 交给上层 handler_
    FrameCodec::OnData(inbuf, [this, conn](const std::string& frame) {
        if (!handler_) return;
        auto rpcConn = std::make_shared<MuduoRpcConnection>(conn);
        handler_(rpcConn, frame);
    });
}

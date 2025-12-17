#include "net_muduo/muduo_network_server.h"
#include <rpc/rpc_meta.pb.h>
#include <rpc/rpc_codec.h>
#include <sstream>
#include <iomanip>
using namespace muduo;
using namespace muduo::net;
using namespace std::placeholders;

MuduoNetworkServer::MuduoNetworkServer(int port, int io_threads)
: loop_(),
  server_(&loop_,
          muduo::net::InetAddress(port),
          "tiny_rpc") {

    // 1) 设置 IO 线程数（>1 会启用 TcpServer 内置的 EventLoopThreadPool）
    server_.setThreadNum(io_threads);

    // 2) 设置连接回调
    server_.setConnectionCallback(
        [this](const muduo::net::TcpConnectionPtr& conn) {
            this->onConnection(conn);
        });

    // 3) 设置消息回调：这里通常先喂给 FrameCodec 拆帧
    server_.setMessageCallback(
        [this](const muduo::net::TcpConnectionPtr& conn,
            muduo::net::Buffer* buf,
            muduo::Timestamp ts) {
            this->onMessage(conn, buf, ts);
        });

    // 4) 这里不要调用 server_.start()，把“启动监听”留给 Run()
}

void MuduoNetworkServer::Run() {
    //监听端口
    server_.start();
    //启动事件循环
    loop_.loop();
}

void MuduoNetworkServer::Stop() {
    loop_.quit();    // 退出事件循环
}

void MuduoNetworkServer::SetMessageHandler(std::shared_ptr<MessageHandler> handler){
    handler_=handler;
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

    /*OnData会解析出frame（因为要出里半包/粘包问题，所以OnData中是while循环解析，在这里注入“回调函数”，每次解析
        出完整一帧frame，就调用一次“回调函数”进行处理）*/
    frame_codec_.OnData(inbuf,std::make_shared<MuduoRpcConnection>(conn),
        [this](const std::shared_ptr<RpcConnection>& conn, const std::string& frame) {
        handler_->HandleMessage(conn, frame);
    });
}

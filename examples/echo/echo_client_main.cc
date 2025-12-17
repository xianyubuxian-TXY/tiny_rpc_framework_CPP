#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Logging.h>

#include "rpc/rpc_channel.h"
#include "rpc/rpc_controller.h"
#include "net/frame_codec.h"
#include "echo.pb.h"
#include <net_muduo/muduo_rpc_connection.h>

using namespace muduo;
using namespace muduo::net;

class EchoClient {
public:
    EchoClient(EventLoop* loop, const InetAddress& serverAddr)
        : client_(loop, serverAddr, "EchoClient"),
          // 初始化 RpcChannel，注入“发送函数”
          channel_([this](const std::string& data) {
              if (conn_ && conn_->connected()) {
                  conn_->send(data);
              }
          }),
          stub_(&channel_)  // 使用 RpcChannel 构造 Stub
    {
        client_.setConnectionCallback(
            std::bind(&EchoClient::onConnection, this, std::placeholders::_1));
        client_.setMessageCallback(
            std::bind(&EchoClient::onMessage, this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
    }

    void Connect() { client_.connect(); }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_INFO << "Connected to " << conn->peerAddress().toIpPort();
            conn_ = conn;
            inbuf_.clear();

            // 连接建立后发起一次 RPC 调用
            SendEcho("Hello from client via Stub!");
        } else {
            LOG_INFO << "Disconnected";
            conn_.reset();
        }
    }

    void onMessage(const TcpConnectionPtr& conn,
                   Buffer* buffer,
                   Timestamp)
    {
        // 用 FrameCodec 按长度前缀拆包，拆出来的 frame 给 RpcChannel
        inbuf_.append(buffer->peek(), buffer->readableBytes());
        buffer->retrieveAll();

        codec_.OnData(inbuf_,std::make_shared<MuduoRpcConnection>(conn),
            [this](const std::shared_ptr<RpcConnection>& conn, const std::string& frame) {
                channel_.OnMessage(frame);
        });

    }

    // 调用 Echo RPC
    void SendEcho(const std::string& text) {
        request_.set_message(text);

        // 使用 Protobuf 的 RpcController & Closure
        controller_.Reset();
        // done 回调，在收到响应后调用
        google::protobuf::Closure* done =
            google::protobuf::NewCallback<EchoClient>(
                this, &EchoClient::OnEchoDone);

        stub_.Echo(&controller_, &request_, &response_, done);
    }

    // RPC 完成回调（在 RpcChannel::OnMessage 中调用）
    void OnEchoDone() {
        if (controller_.Failed()) {
            LOG_ERROR << "RPC failed: " << controller_.ErrorText();
        } else {
            LOG_INFO << "RPC response: " << response_.message();
        }

        // 示例：收完一次就关闭连接
        if (conn_) {
            conn_->shutdown();
        }
    }

private:
    TcpClient client_;
    TcpConnectionPtr conn_;
    FrameCodec codec_;

    std::string inbuf_; // 来自这个连接的接收缓冲区

    // RPC 相关
    SimpleRpcChannel channel_;            // 通道
    demo::EchoService_Stub stub_;        // 业务 Stub
    demo::EchoRequest request_;          // 请求 DTO
    demo::EchoResponse response_;        // 响应 DTO
    SimpleRpcController controller_; // 控制器
};

int main() {
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 12345);

    EchoClient client(&loop, serverAddr);
    client.Connect();
    loop.loop();
    return 0;
}

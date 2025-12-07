#pragma once
#include "rpc/rpc_connection.h"
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>

class MuduoRpcConnection : public RpcConnection {
public:
    explicit MuduoRpcConnection(const muduo::net::TcpConnectionPtr& conn)
        : conn_(conn) {}

    void Send(const std::string& data) override {
        if (!conn_) {
            LOG_ERROR << "RpcConnection null";
            return;
        }
        if (!conn_->connected()) {
            LOG_WARN << "RpcConnection disconnected, drop response";
            return;
        }
        conn_->send(data);
    }

private:
    muduo::net::TcpConnectionPtr conn_;
};

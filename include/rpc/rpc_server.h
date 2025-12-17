#pragma once
#include <memory>
#include "net/network_server.h"
#include "rpc_dispatcher.h"

/*用于服务端*/
class RpcServer {
public:
    explicit RpcServer(std::unique_ptr<INetworkServer> net)
        : network_(std::move(net)),
            dispatcher_(std::make_shared<RpcDispatcher>()) {
        network_->SetMessageHandler(dispatcher_);
    }

    void RegisterService(google::protobuf::Service* service) {
        dispatcher_->RegisterService(service);
    }

    void Run() { network_->Run(); }
    void Stop()  { network_->Stop(); }

private:
    std::unique_ptr<INetworkServer> network_;
    std::shared_ptr<RpcDispatcher> dispatcher_;
};
    
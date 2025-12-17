#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <google/protobuf/service.h>
#include "rpc_meta.pb.h"
#include "rpc/rpc_connection.h"
#include "net/network_server.h"

/*因为RpcDispatcher要处理网络层的frame，所以继承MessageHandler*/
class RpcDispatcher: public MessageHandler {
public:
    void RegisterService(google::protobuf::Service* service);
    //重载HandleMessage
    void HandleMessage(const std::shared_ptr<RpcConnection>& conn,
        const std::string& frame) override;
private:
    void OnRpcMessage(const std::shared_ptr<RpcConnection>& conn,
                                      const rpc::RpcMeta& meta,
                                      const std::string& payload);

    std::unordered_map<std::string, google::protobuf::Service*> services_;
};

#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <google/protobuf/service.h>
#include "rpc_meta.pb.h"
#include "rpc/rpc_connection.h"

class RpcDispatcher {
public:
    void RegisterService(google::protobuf::Service* service);
    void OnRpcMessage(RpcConnection* conn,
                      const rpc::RpcMeta& meta,
                      const std::string& payload);

private:
    std::unordered_map<std::string, google::protobuf::Service*> services_;
};

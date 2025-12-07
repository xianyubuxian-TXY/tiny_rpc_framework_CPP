#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <string>

#include "rpc_meta.pb.h"

class SimpleRpcChannel : public google::protobuf::RpcChannel {
public:
    using SendFunction = std::function<void(const std::string&)>;

    explicit SimpleRpcChannel(SendFunction send);

    // protobuf Stub 调用时会走到这里
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done) override;

    // 由网络层在收到“响应帧”时调用
    void OnMessage(const std::string& frame);

private:
    struct PendingCall {
        google::protobuf::Message* response;   // 由调用方分配，RpcChannel 只负责填充
        google::protobuf::Closure* done;       // 完成后调用 done->Run()
        google::protobuf::RpcController* controller; // 可选，用于设置错误
    };

    uint64_t NextRequestId();

    std::mutex mutex_;
    uint64_t next_id_ = 1;
    std::unordered_map<uint64_t, PendingCall> pending_calls_;
    SendFunction send_;
};

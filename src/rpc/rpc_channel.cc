#include "rpc/rpc_channel.h"
#include "rpc/rpc_codec.h"

#include <google/protobuf/descriptor.h>
#include <iostream>

using namespace google::protobuf;

SimpleRpcChannel::SimpleRpcChannel(SendFunction send)
    :send_(std::move(send))
{}

uint64_t SimpleRpcChannel::NextRequestId() {
    // 简单递增，不考虑溢出（生产中可以做更严谨处理）
    return next_id_++;
}

void SimpleRpcChannel::CallMethod(const MethodDescriptor* method,
                            RpcController* controller,
                            const Message* request,
                            Message* response,
                            Closure* done)
{
    if (!send_) {
        if (controller) {
            controller->SetFailed("No send function set in RpcChannel");
        }
        if (done) done->Run();
        return;
    }

    // 1. 准备 RpcMeta
    const ServiceDescriptor* svc_desc = method->service();
    rpc::RpcMeta meta;
    meta.set_service_name(svc_desc->full_name());
    meta.set_method_name(method->name());
    meta.set_is_request(true);

    uint64_t req_id = NextRequestId();
    meta.set_request_id(static_cast<uint64_t>(req_id));

    // 2. 编码 frame (meta + body)
    std::string frame;
    if (!RpcCodec::EncodeFrame(meta, *request, &frame)) {
        if (controller) {
            controller->SetFailed("RpcCodec::EncodeFrame failed");
        }
        if (done) done->Run();
        return;
    }

    // 3. 保存 pending call
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_calls_[req_id] = PendingCall{ response, done, controller };
    }

    // 4. 加长度前缀并发送
    std::string out = RpcCodec::AddLengthPrefix(frame);
    send_(out);
}

// 网络层收到一帧数据后调用
void SimpleRpcChannel::OnMessage(const std::string& frame)
{
    rpc::RpcMeta meta;
    std::string payload;
    if (!RpcCodec::DecodeFrame(frame, &meta, &payload)) {
        std::cerr << "RpcChannel::OnMessage: DecodeFrame failed" << std::endl;
        return;
    }

    if (meta.is_request()) {
        // Channel 只处理“响应”，请求是发给服务端的
        std::cerr << "RpcChannel::OnMessage: got request frame in client channel" << std::endl;
        return;
    }

    uint64_t req_id = meta.request_id();

    PendingCall call;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pending_calls_.find(req_id);
        if (it == pending_calls_.end()) {
            std::cerr << "RpcChannel::OnMessage: unknown request_id = "
                      << req_id << std::endl;
            return;
        }
        call = it->second;
        pending_calls_.erase(it);
    }

    // 检查是否有错误码
    if (meta.error_code() != 0) {
        if (call.controller) {
            call.controller->SetFailed(meta.error_msg());
        }
        if (call.done) {
            call.done->Run();
        }
        return;
    }

    // 解析响应体
    if (!call.response->ParseFromString(payload)) {
        if (call.controller) {
            call.controller->SetFailed("Parse response message failed");
        }
        if (call.done) {
            call.done->Run();
        }
        return;
    }

    // 正常完成回调
    if (call.done) {
        call.done->Run();
    }
}

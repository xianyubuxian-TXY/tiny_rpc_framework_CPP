#include "rpc/rpc_dispatcher.h"
#include "rpc/rpc_codec.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <iostream>

using namespace google::protobuf;

void RpcDispatcher::RegisterService(Service* service) {
    const ServiceDescriptor* desc = service->GetDescriptor();
    std::string name = desc->full_name();

    if (services_.count(name)) {
        std::cerr << "Service already registered: " << name << std::endl;
        return;
    }
    services_[name] = service;
}

void RpcDispatcher::OnRpcMessage(RpcConnection* conn,
                                 const rpc::RpcMeta& meta,
                                 const std::string& payload)
{
    auto it = services_.find(meta.service_name());
    if (it == services_.end()) {
        std::cerr << "Unknown service: " << meta.service_name() << std::endl;
        return;
    }

    Service* service = it->second;
    const ServiceDescriptor* svc_desc = service->GetDescriptor();
    const MethodDescriptor* method =
        svc_desc->FindMethodByName(meta.method_name());

    if (!method) {
        std::cerr << "Unknown method: " << meta.method_name()
                  << " in service " << meta.service_name() << std::endl;
        return;
    }

    std::unique_ptr<Message> request(
        MessageFactory::generated_factory()
            ->GetPrototype(method->input_type())->New());
    std::unique_ptr<Message> response(
        MessageFactory::generated_factory()
            ->GetPrototype(method->output_type())->New());

    if (!request->ParseFromString(payload)) {
        std::cerr << "Failed to parse request payload for "
                  << meta.service_name() << "." << meta.method_name()
                  << std::endl;
        return;
    }

    class DummyController : public RpcController {
    public:
        void Reset() override {}
        bool Failed() const override { return false; }
        std::string ErrorText() const override { return {}; }
        void StartCancel() override {}
        void SetFailed(const std::string& reason) override { (void)reason; }
        bool IsCanceled() const override { return false; }
        void NotifyOnCancel(Closure* callback) override { (void)callback; }
    } controller;

    service->CallMethod(method, &controller, request.get(), response.get(), nullptr);

    rpc::RpcMeta rsp_meta;
    rsp_meta.set_service_name(meta.service_name());
    rsp_meta.set_method_name(meta.method_name());
    rsp_meta.set_request_id(meta.request_id());
    rsp_meta.set_is_request(false);
    rsp_meta.set_error_code(0);
    rsp_meta.set_error_msg("");

    std::string frame;
    if (!RpcCodec::EncodeFrame(rsp_meta, *response, &frame)) {
        std::cerr << "Failed to encode response" << std::endl;
        return;
    }

    // 这里发送的是“frame”，由具体网络层再加长度前缀
    std::string out = RpcCodec::AddLengthPrefix(frame);
    conn->Send(out);
}

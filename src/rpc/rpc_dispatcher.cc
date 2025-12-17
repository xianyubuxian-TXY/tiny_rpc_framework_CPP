#include "rpc/rpc_dispatcher.h"
#include "rpc/rpc_codec.h"
#include <google/protobuf/descriptor.h>  // Protobuf服务/方法描述符头文件
#include <google/protobuf/message.h>      // Protobuf消息基类头文件
#include <iostream>
#include <muduo/base/Logging.h>

using namespace google::protobuf;

// RpcDispatcher：RPC请求分发器核心类
// 核心职责：注册RPC服务、接收RPC请求、路由到具体服务方法、执行并返回响应

/**
 * @brief 注册RPC服务到分发器的服务注册表
 * @param service 待注册的RPC服务实例（Protobuf自动生成的Service子类，如OrderService）
 */
void RpcDispatcher::RegisterService(Service* service) {
    // 1. 获取服务的描述符（Protobuf自动生成的元信息，包含服务全名、方法列表等）
    const ServiceDescriptor* desc = service->GetDescriptor();
    // 2. 获取服务的全限定名（如"order.OrderService"），作为注册表的Key
    std::string name = desc->full_name();

    // 3. 检查服务是否已注册，避免重复注册导致冲突
    if (services_.count(name)) {
        std::cerr << "Service already registered: " << name << std::endl;
        return;
    }
    // 4. 将服务实例存入注册表（services_应为类成员，类型：std::unordered_map<std::string, Service*>）
    services_[name] = service;
}

void RpcDispatcher::HandleMessage(const std::shared_ptr<RpcConnection>& conn,
                                        const std::string& frame)
{
    rpc::RpcMeta meta;
    std::string payload;
    //解析出meta、payload
    if (!RpcCodec::DecodeFrame(frame, &meta, &payload)) {
        std::cerr << "Dispatcher DecodeFrame failed, frame.size="
                  << frame.size() << std::endl;
        return;
    }
    // LOG_INFO << "Dispatcher got service=" << meta.service_name()
    //      << " method=" << meta.method_name()
    //      << " req_id=" << meta.request_id();
    OnRpcMessage(conn,meta,payload);
}


/**
 * @brief 处理解析后的RPC请求（核心方法）
 * @param conn 对应的RPC网络连接（用于回写响应）
 * @param meta RPC元信息（包含服务名、方法名、request_id等）
 * @param payload 序列化后的请求消息体（二进制字符串）
 */
void RpcDispatcher::OnRpcMessage(const std::shared_ptr<RpcConnection>& conn,
                                 const rpc::RpcMeta& meta,
                                 const std::string& payload)
{
    // ===================== 步骤1：查找已注册的服务 =====================
    auto it = services_.find(meta.service_name());
    if (it == services_.end()) {
        std::cerr << "Unknown service: " << meta.service_name() << std::endl;
        return;
    }
    // 找到服务实例
    Service* service = it->second;

    // ===================== 步骤2：查找服务对应的方法 =====================
    // 获取服务描述符
    const ServiceDescriptor* svc_desc = service->GetDescriptor();
    // 通过方法名查找方法描述符（如"GetOrder"）
    const MethodDescriptor* method =
        svc_desc->FindMethodByName(meta.method_name());

    if (!method) {
        std::cerr << "Unknown method: " << meta.method_name()
                  << " in service " << meta.service_name() << std::endl;
        return;
    }

    // ===================== 步骤3：创建请求/响应消息对象 =====================
    // 1. 创建请求消息对象：通过方法描述符获取输入类型原型，再新建实例
    std::unique_ptr<Message> request(
        MessageFactory::generated_factory()  // Protobuf自动生成的消息工厂
            ->GetPrototype(method->input_type())  // 获取请求消息原型（如GetOrderRequest）
            ->New());  // 创建空的请求消息对象
    // 2. 创建响应消息对象：同理，获取输出类型原型并新建实例
    std::unique_ptr<Message> response(
        MessageFactory::generated_factory()
            ->GetPrototype(method->output_type())  // 获取响应消息原型（如GetOrderResponse）
            ->New());

    // ===================== 步骤4：解析请求消息体 =====================
    // 将二进制payload反序列化为请求消息对象
    if (!request->ParseFromString(payload)) {
        std::cerr << "Failed to parse request payload for "
                  << meta.service_name() << "." << meta.method_name()
                  << std::endl;
        return;
    }

    // ===================== 步骤5：定义RpcController（控制器） =====================
    // RpcController：Protobuf RPC标准接口，用于传递调用状态（错误、取消等）
    // 此处为哑实现（Dummy），仅满足接口要求，生产环境需扩展实际逻辑
    class DummyController : public RpcController {
    public:
        void Reset() override {}  // 重置控制器状态
        bool Failed() const override { return false; }  // 调用是否失败
        std::string ErrorText() const override { return {}; }  // 错误信息
        void StartCancel() override {}  // 取消调用
        void SetFailed(const std::string& reason) override { (void)reason; }  // 设置失败原因
        bool IsCanceled() const override { return false; }  // 是否被取消
        void NotifyOnCancel(Closure* callback) override { (void)callback; }  // 取消回调
    } controller;

    // ===================== 步骤6：执行RPC服务方法 =====================
    // CallMethod：Protobuf自动生成的方法调用入口（同步调用）
    // 参数说明：
    // - method：要执行的方法描述符
    // - &controller：控制器（传递调用状态）
    // - request.get()：解析后的请求消息
    // - response.get()：空响应消息（执行后填充结果）
    // - nullptr：异步回调（同步调用设为null）
    service->CallMethod(method, &controller, request.get(), response.get(), nullptr);

    // ===================== 步骤7：封装响应元信息 =====================
    rpc::RpcMeta rsp_meta;
    rsp_meta.set_service_name(meta.service_name());  // 复用请求的服务名
    rsp_meta.set_method_name(meta.method_name());    // 复用请求的方法名
    rsp_meta.set_request_id(meta.request_id());      // 复用request_id，保证客户端配对
    rsp_meta.set_is_request(false);                  // 标记为响应（非请求）
    rsp_meta.set_error_code(0);                      // 0表示成功（生产环境需从controller获取错误码）
    rsp_meta.set_error_msg("");                      // 错误信息（默认空）

    // ===================== 步骤8：序列化响应并发送 =====================
    // 存储序列化后的响应帧
    std::string frame;
    // 1. 将响应元信息+响应消息序列化为完整帧（RpcCodec核心功能）
    if (!RpcCodec::EncodeFrame(rsp_meta, *response, &frame)) {
        std::cerr << "Failed to encode response" << std::endl;
        return;
    }

    // 2. 添加长度前缀（解决网络粘包/拆包问题）
    // AddLengthPrefix：在帧前添加4字节长度值，标识后续数据长度
    std::string out = RpcCodec::AddLengthPrefix(frame);
    // 3. 通过网络连接发送响应数据给客户端
    conn->Send(out);
}
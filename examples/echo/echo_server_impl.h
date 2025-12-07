#pragma once
#include "echo.pb.h"

// 业务实现：只关心 EchoRequest / EchoResponse（DTO），不关心网络
class EchoServiceImpl : public demo::EchoService {
public:
    void Echo(::google::protobuf::RpcController* controller,
              const ::demo::EchoRequest* request,
              ::demo::EchoResponse* response,
              ::google::protobuf::Closure* done) override
    {
        (void)controller;
        // 简单 echo + 加一点业务逻辑
        response->set_message("server echo: " + request->message());

        if (done) done->Run(); // 这里我们是同步实现，可以不调用也行
    }
};

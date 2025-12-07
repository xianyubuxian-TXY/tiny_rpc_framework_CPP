#pragma once
#include <string>

class RpcConnection {
public:
    virtual ~RpcConnection() = default;
    virtual void Send(const std::string& data) = 0;
};

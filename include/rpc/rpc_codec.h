#pragma once
#include "rpc_meta.pb.h"
#include <google/protobuf/message.h>
#include <string>

class RpcCodec {
public:
    // 编码：RpcMeta + Message => frame（二进制，不包含总长度前缀）
    static bool EncodeFrame(const rpc::RpcMeta& meta,
                            const google::protobuf::Message& msg,
                            std::string* out);

    // 解码：frame（二进制） => RpcMeta + payload bytes
    static bool DecodeFrame(const std::string& frame,
                            rpc::RpcMeta* meta,
                            std::string* payload);

    // 可选：把 frame 再加一个 length 前缀，方便在客户端用
    static std::string AddLengthPrefix(const std::string& frame);
};

#include "rpc/rpc_codec.h"
#include <arpa/inet.h>

//序列化：meta、msg——>out([meta_len][meta][payload])
bool RpcCodec::EncodeFrame(const rpc::RpcMeta& meta,
                           const google::protobuf::Message& msg,
                           std::string* out)
{
    std::string meta_bytes;
    if (!meta.SerializeToString(&meta_bytes)) {
        return false;
    }

    std::string body_bytes;
    if (!msg.SerializeToString(&body_bytes)) {
        return false;
    }

    uint32_t meta_len = meta_bytes.size();

    out->clear();
    out->reserve(4 + meta_bytes.size() + body_bytes.size());

    uint32_t meta_len_net = htonl(meta_len);
    out->append(reinterpret_cast<const char*>(&meta_len_net), 4);
    out->append(meta_bytes);
    out->append(body_bytes);

    return true;
}

//反序列化：frame([meta_len][meta][payload])——>提取 meta、payload
bool RpcCodec::DecodeFrame(const std::string& frame,
                           rpc::RpcMeta* meta,
                           std::string* payload)
{
    if (frame.size() < 4) return false;

    uint32_t meta_len_net = 0;
    std::memcpy(&meta_len_net, frame.data(), 4);
    uint32_t meta_len = ntohl(meta_len_net);

    if (frame.size() < 4 + meta_len) return false;

    std::string meta_bytes = frame.substr(4, meta_len);
    if (!meta->ParseFromString(meta_bytes)) {
        return false;
    }

    *payload = frame.substr(4 + meta_len);
    return true;
}

//增加“总长度”前缀：[meta_len][meta][payload]——>[total_len][meta_len][meta][payload]
std::string RpcCodec::AddLengthPrefix(const std::string& frame)
{
    uint32_t total_len = static_cast<uint32_t>(frame.size());
    uint32_t net_len = htonl(total_len);

    std::string out;
    out.reserve(4 + frame.size());
    out.append(reinterpret_cast<const char*>(&net_len), 4);
    out.append(frame);
    return out;
}

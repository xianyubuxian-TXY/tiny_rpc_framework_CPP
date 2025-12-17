#pragma once
#include <string>
#include <functional>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include "network_server.h"

class FrameCodec {
public:
    using FrameCallback = std::function<void(const std::shared_ptr<RpcConnection>& conn,
                                                    const std::string& frame)>;

    /*为什么OnData静态方法不直接依赖MessageHandler？而要使用“回调函数”，是否多此一举？
        使用“回调函数”是为了“解耦”
        FrameCodec 是一种 解码器，它应该负责 按协议解析数据，但它不应该知道 具体的业务处理逻辑。MessageHandler 是“业务层”的抽象，应该由上层注入，而不是在解码器中直接耦合。
        如果 FrameCodec 直接依赖 MessageHandler，会导致：
        解码逻辑和业务处理逻辑耦合
        难以替换 MessageHandler，或者需要修改 FrameCodec 中的代码
        难以进行单元测试和解耦
    */
                                                
    // 处理 buffer 中的数据，按 [4字节total_len][...payload...] 拆包
    // - buffer 是某个连接的接收缓冲区字符串
    // - cb: 每解析出一条完整 frame 调用一次
    void OnData(std::string& buffer,const std::shared_ptr<RpcConnection>& conn,const FrameCallback& cb) {
        constexpr size_t kHeaderLen = 4;

        while (true) {
            if (buffer.size() < kHeaderLen) return;

            uint32_t len_net = 0;
            std::memcpy(&len_net, buffer.data(), kHeaderLen);
            uint32_t len = ntohl(len_net);

            LOG_INFO << "FrameCodec buffer.size=" << buffer.size()
            << " total_len=" << len
            << " need=" << (4 + len);

            if (buffer.size() < kHeaderLen + len) {
                // 半包，等待更多数据
                return;
            }

            std::string frame = buffer.substr(kHeaderLen, len);
            buffer.erase(0, kHeaderLen + len);

            cb(conn,frame); // 交给上层
        }
    }
};

#pragma once
#include <string>
#include <functional>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>

class FrameCodec {
public:
    using FrameCallback = std::function<void(const std::string& frame)>;

    // 处理 buffer 中的数据，按 [4字节total_len][...payload...] 拆包
    // - buffer 是某个连接的接收缓冲区字符串
    // - cb: 每解析出一条完整 frame 调用一次
    static void OnData(std::string& buffer, const FrameCallback& cb) {
        constexpr size_t kHeaderLen = 4;

        while (true) {
            if (buffer.size() < kHeaderLen) return;

            uint32_t len_net = 0;
            std::memcpy(&len_net, buffer.data(), kHeaderLen);
            uint32_t len = ntohl(len_net);

            if (buffer.size() < kHeaderLen + len) {
                // 半包，等待更多数据
                return;
            }

            std::string frame = buffer.substr(kHeaderLen, len);
            buffer.erase(0, kHeaderLen + len);

            cb(frame); // 交给上层
        }
    }
};

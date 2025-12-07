#pragma once

#include <google/protobuf/service.h>
#include <string>

class SimpleRpcController : public google::protobuf::RpcController {
public:
    SimpleRpcController() { Reset(); }
    ~SimpleRpcController() override = default;

    // 重新开始一次调用
    void Reset() override {
        failed_ = false;
        error_text_.clear();
    }

    // 是否失败
    bool Failed() const override {
        return failed_;
    }

    // 错误文本
    std::string ErrorText() const override {
        return error_text_;
    }

    // 取消相关，这里我们不做真正的取消逻辑，留空实现即可
    void StartCancel() override {
        // no-op
    }

    // 标记失败
    void SetFailed(const std::string& reason) override {
        failed_ = true;
        error_text_ = reason;
    }

    // 是否被取消，这里直接返回 false
    bool IsCanceled() const override {
        return false;
    }

    // 注册取消回调，这里不支持取消，直接释放回调（避免泄露）
    void NotifyOnCancel(google::protobuf::Closure* callback) override {
        if (callback) {
            delete callback;
        }
    }

private:
    bool failed_{false};
    std::string error_text_;
};
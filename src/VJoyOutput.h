#pragma once

#include <string>

enum class VJoyAxis { X, Y };

class VJoyOutput {
public:
    ~VJoyOutput();
    VJoyOutput() = default;
    VJoyOutput(const VJoyOutput&) = delete;
    VJoyOutput& operator=(const VJoyOutput&) = delete;

    bool initialize(unsigned int deviceId = 1);
    bool setAxis(VJoyAxis axis, long position);
    void centre(VJoyAxis axis);
    void shutdown();
    bool active() const { return active_; }
    const std::wstring& status() const { return status_; }

    static constexpr long minimum() { return 1; }
    static constexpr long maximum() { return 0x8000; }
    static constexpr long centrePosition() { return 0x4000; }

private:
    unsigned int deviceId_ = 1;
    bool active_ = false;
    std::wstring status_ = L"vJoy unavailable";
    void* module_ = nullptr;
    bool (__cdecl* vJoyEnabled_)() = nullptr;
    bool (__cdecl* acquireVjd_)(unsigned int) = nullptr;
    void (__cdecl* relinquishVjd_)(unsigned int) = nullptr;
    bool (__cdecl* setAxis_)(long, unsigned int, unsigned int) = nullptr;
};

#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

struct InterceptionMouseDevice {
    int id = 0;
    std::wstring hardwareId;
};

struct InterceptionKeyboardDevice {
    int id = 0;
    std::wstring hardwareId;
};

struct InterceptionMouseStroke {
    unsigned short state = 0;
    unsigned short flags = 0;
    short rolling = 0;
    int x = 0;
    int y = 0;
    unsigned int information = 0;
};

// Receives mouse strokes below the normal Windows input stack. Strokes from
// the selected device are deliberately not forwarded; every other mouse is.
class InterceptionInput {
public:
    using MovementCallback = std::function<void(int x, int y)>;
    using KeyCallback = std::function<void(unsigned short code, unsigned short state)>;
    using RawMouseCallback = std::function<void(int deviceId, const InterceptionMouseStroke& stroke)>;
    ~InterceptionInput();
    InterceptionInput() = default;
    InterceptionInput(const InterceptionInput&) = delete;
    InterceptionInput& operator=(const InterceptionInput&) = delete;

    bool start(MovementCallback callback, KeyCallback keyCallback = {}, RawMouseCallback rawMouseCallback = {});
    void selectMouse(int interceptionDeviceId);
    void selectKeyboard(int interceptionDeviceId);
    std::vector<InterceptionMouseDevice> mice() const;
    std::vector<InterceptionKeyboardDevice> keyboards() const;
    void stop();
    bool active() const { return active_; }
    const std::wstring& status() const { return status_; }

private:
    void run();
    void* module_ = nullptr;
    void* context_ = nullptr;
    std::thread worker_;
    std::atomic<bool> running_ = false;
    std::atomic<int> selectedDevice_ = 0;
    std::atomic<int> selectedKeyboardDevice_ = 0;
    bool active_ = false;
    std::wstring status_ = L"Interception unavailable";
    MovementCallback callback_;
    KeyCallback keyCallback_;
    RawMouseCallback rawMouseCallback_;

    void* (__cdecl* createContext_)() = nullptr;
    void (__cdecl* destroyContext_)(void*) = nullptr;
    void (__cdecl* setFilter_)(void*, int (__cdecl*)(int), unsigned short) = nullptr;
    int (__cdecl* waitWithTimeout_)(void*, unsigned long) = nullptr;
    int (__cdecl* receive_)(void*, int, void*, unsigned int) = nullptr;
    int (__cdecl* send_)(void*, int, const void*, unsigned int) = nullptr;
    unsigned int (__cdecl* hardwareId_)(void*, int, void*, unsigned int) = nullptr;
};

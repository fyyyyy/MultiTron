#include "InterceptionInput.h"

#include <array>
#include <windows.h>

namespace {
constexpr int kFirstMouseDevice = 11;
constexpr int kLastMouseDevice = 20;
constexpr int kFirstKeyboardDevice = 1;
constexpr int kLastKeyboardDevice = 10;
constexpr unsigned short kFilterMouseAll = 0xFFFF;

struct KeyStroke {
    unsigned short code;
    unsigned short state;
    unsigned int information;
};

int __cdecl isMouse(int device) {
    return device >= kFirstMouseDevice && device <= kLastMouseDevice;
}

int __cdecl isKeyboard(int device) {
    return device >= kFirstKeyboardDevice && device <= kLastKeyboardDevice;
}

int __cdecl isInput(int device) { return isMouse(device) || isKeyboard(device); }
}

InterceptionInput::~InterceptionInput() { stop(); }

bool InterceptionInput::start(MovementCallback callback, KeyCallback keyCallback, RawMouseCallback rawMouseCallback) {
    stop();
    const wchar_t* locations[] = {
        L"interception.dll",
        L"third_party\\interception\\x64\\interception.dll",
    };
    HMODULE module = nullptr;
    for (const auto* location : locations) {
        module = LoadLibraryW(location);
        if (module) break;
    }
    if (!module) { status_ = L"interception.dll was not found"; return false; }
    module_ = module;
    createContext_ = reinterpret_cast<void* (__cdecl*)()>(GetProcAddress(module, "interception_create_context"));
    destroyContext_ = reinterpret_cast<void (__cdecl*)(void*)>(GetProcAddress(module, "interception_destroy_context"));
    setFilter_ = reinterpret_cast<void (__cdecl*)(void*, int (__cdecl*)(int), unsigned short)>(GetProcAddress(module, "interception_set_filter"));
    waitWithTimeout_ = reinterpret_cast<int (__cdecl*)(void*, unsigned long)>(GetProcAddress(module, "interception_wait_with_timeout"));
    receive_ = reinterpret_cast<int (__cdecl*)(void*, int, void*, unsigned int)>(GetProcAddress(module, "interception_receive"));
    send_ = reinterpret_cast<int (__cdecl*)(void*, int, const void*, unsigned int)>(GetProcAddress(module, "interception_send"));
    hardwareId_ = reinterpret_cast<unsigned int (__cdecl*)(void*, int, void*, unsigned int)>(GetProcAddress(module, "interception_get_hardware_id"));
    if (!createContext_ || !destroyContext_ || !setFilter_ || !waitWithTimeout_ || !receive_ || !send_ || !hardwareId_) {
        status_ = L"Interception DLL is incompatible"; stop(); return false;
    }
    context_ = createContext_();
    if (!context_) { status_ = L"Interception driver is unavailable; reboot may be required"; stop(); return false; }
    setFilter_(context_, isInput, kFilterMouseAll);
    callback_ = std::move(callback);
    keyCallback_ = std::move(keyCallback);
    rawMouseCallback_ = std::move(rawMouseCallback);
    running_ = true;
    active_ = true;
    status_ = L"Interception active - selected mouse will be suppressed";
    worker_ = std::thread(&InterceptionInput::run, this);
    return true;
}

void InterceptionInput::selectMouse(int interceptionDeviceId) { selectedDevice_ = interceptionDeviceId; }
void InterceptionInput::selectKeyboard(int interceptionDeviceId) { selectedKeyboardDevice_ = interceptionDeviceId; }

std::vector<InterceptionMouseDevice> InterceptionInput::mice() const {
    std::vector<InterceptionMouseDevice> devices;
    if (!context_ || !hardwareId_) return devices;
    for (int id = kFirstMouseDevice; id <= kLastMouseDevice; ++id) {
        std::array<wchar_t, 512> hardware{};
        const unsigned int length = hardwareId_(context_, id, hardware.data(), sizeof(hardware));
        if (length == 0) continue;
        devices.push_back({id, std::wstring(hardware.data())});
    }
    return devices;
}

std::vector<InterceptionKeyboardDevice> InterceptionInput::keyboards() const {
    std::vector<InterceptionKeyboardDevice> devices;
    if (!context_ || !hardwareId_) return devices;
    for (int id = kFirstKeyboardDevice; id <= kLastKeyboardDevice; ++id) {
        std::array<wchar_t, 512> hardware{};
        const unsigned int length = hardwareId_(context_, id, hardware.data(), sizeof(hardware));
        if (length == 0) continue;
        devices.push_back({id, std::wstring(hardware.data())});
    }
    return devices;
}

void InterceptionInput::run() {
    while (running_) {
        const int device = waitWithTimeout_(context_, 50);
        if (!running_ || device == 0) continue;
        if (isMouse(device)) {
            InterceptionMouseStroke stroke{};
            if (receive_(context_, device, &stroke, 1) <= 0) continue;
            if (rawMouseCallback_) rawMouseCallback_(device, stroke);
            if (device == selectedDevice_) {
                if (callback_) callback_(stroke.x, stroke.y);
                continue; // Bike-mouse movement is consumed here.
            }
            send_(context_, device, &stroke, 1);
        } else {
            KeyStroke stroke{};
            if (receive_(context_, device, &stroke, 1) <= 0) continue;
            if (device == selectedKeyboardDevice_) {
                if (keyCallback_) keyCallback_(stroke.code, stroke.state);
                continue; // Loudness-controller keys are consumed here.
            }
            send_(context_, device, &stroke, 1);
        }
    }
}

void InterceptionInput::stop() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
    active_ = false;
    selectedDevice_ = 0;
    selectedKeyboardDevice_ = 0;
    if (context_ && destroyContext_) destroyContext_(context_);
    context_ = nullptr;
    if (module_) FreeLibrary(static_cast<HMODULE>(module_));
    module_ = nullptr;
    createContext_ = nullptr; destroyContext_ = nullptr; setFilter_ = nullptr; waitWithTimeout_ = nullptr;
    receive_ = nullptr; send_ = nullptr; hardwareId_ = nullptr;
    callback_ = {};
    keyCallback_ = {};
    rawMouseCallback_ = {};
}

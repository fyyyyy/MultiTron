#include "VJoyOutput.h"

#include <algorithm>
#include <windows.h>

namespace {
constexpr unsigned int kHidUsageX = 0x30;
constexpr unsigned int kHidUsageY = 0x31;
}

VJoyOutput::~VJoyOutput() { shutdown(); }

bool VJoyOutput::initialize(unsigned int deviceId) {
    shutdown();
    deviceId_ = deviceId;
    const wchar_t* locations[] = {
        L"vJoyInterface.dll",
        L"C:\\Program Files\\vJoy\\x64\\vJoyInterface.dll",
        L"C:\\Program Files (x86)\\vJoy\\x64\\vJoyInterface.dll",
    };
    HMODULE module = nullptr;
    for (const auto* location : locations) {
        module = LoadLibraryW(location);
        if (module) break;
    }
    if (!module) { status_ = L"vJoy driver/DLL not installed"; return false; }

    module_ = module;
    vJoyEnabled_ = reinterpret_cast<bool (__cdecl*)()>(GetProcAddress(module, "vJoyEnabled"));
    acquireVjd_ = reinterpret_cast<bool (__cdecl*)(unsigned int)>(GetProcAddress(module, "AcquireVJD"));
    relinquishVjd_ = reinterpret_cast<void (__cdecl*)(unsigned int)>(GetProcAddress(module, "RelinquishVJD"));
    setAxis_ = reinterpret_cast<bool (__cdecl*)(long, unsigned int, unsigned int)>(GetProcAddress(module, "SetAxis"));
    if (!vJoyEnabled_ || !acquireVjd_ || !relinquishVjd_ || !setAxis_) {
        status_ = L"vJoy DLL is incompatible"; shutdown(); return false;
    }
    if (!vJoyEnabled_()) { status_ = L"vJoy driver is disabled"; shutdown(); return false; }
    if (!acquireVjd_(deviceId_)) { status_ = L"vJoy Device 1 is busy or missing"; shutdown(); return false; }

    active_ = true;
    status_ = L"vJoy Device 1 active";
    centre(VJoyAxis::X);
    centre(VJoyAxis::Y);
    return true;
}

bool VJoyOutput::setAxis(VJoyAxis axis, long position) {
    if (!active_) return false;
    const unsigned int usage = axis == VJoyAxis::X ? kHidUsageX : kHidUsageY;
    if (!setAxis_(std::clamp(position, minimum(), maximum()), deviceId_, usage)) {
        status_ = L"vJoy axis update failed";
        return false;
    }
    return true;
}

void VJoyOutput::centre(VJoyAxis axis) { setAxis(axis, centrePosition()); }

void VJoyOutput::shutdown() {
    if (active_ && relinquishVjd_) {
        centre(VJoyAxis::X);
        centre(VJoyAxis::Y);
        relinquishVjd_(deviceId_);
    }
    active_ = false;
    if (module_) FreeLibrary(static_cast<HMODULE>(module_));
    module_ = nullptr;
    vJoyEnabled_ = nullptr;
    acquireVjd_ = nullptr;
    relinquishVjd_ = nullptr;
    setAxis_ = nullptr;
}

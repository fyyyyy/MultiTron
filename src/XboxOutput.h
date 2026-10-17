#pragma once

#include <string>
#include <windows.h>
#include <ViGEm/Client.h>

class XboxOutput {
public:
    ~XboxOutput();
    bool initialize();
    void setAxis(int axis, long vjoyPosition, bool flipTriggers = false);
    void shutdown();
    bool active() const { return active_; }
    const std::wstring& status() const { return status_; }

private:
    void* module_ = nullptr;
    void* client_ = nullptr;
    void* target_ = nullptr;
    bool active_ = false;
    XUSB_REPORT report_{};
    std::wstring status_ = L"Xbox output disabled";
};

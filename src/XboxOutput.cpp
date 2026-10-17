#include "XboxOutput.h"
#include "VJoyOutput.h"

#include <windows.h>
#include <ViGEm/Client.h>

#include <algorithm>
#include <cmath>

XboxOutput::~XboxOutput() { shutdown(); }

bool XboxOutput::initialize() {
    shutdown();
    const auto client = vigem_alloc();
    const auto target = client ? vigem_target_x360_alloc() : nullptr;
    if (!client || !target || vigem_connect(client) != VIGEM_ERROR_NONE
        || vigem_target_add(client, target) != VIGEM_ERROR_NONE) {
        if (target) vigem_target_free(target);
        if (client) vigem_free(client);
        status_ = L"ViGEmBus is unavailable";
        return false;
    }
    client_ = client;
    target_ = target;
    active_ = true;
    status_ = L"Xbox 360 controller active";
    return true;
}

void XboxOutput::setAxis(int axis, long position, bool flipTriggers) {
    if (!active_) return;
    const double fraction = (position - VJoyOutput::centrePosition())
        / static_cast<double>(VJoyOutput::maximum() - VJoyOutput::centrePosition());
    const double clampedFraction = std::clamp(fraction, -1.0, 1.0);
    const SHORT value = static_cast<SHORT>(clampedFraction * 32767.0);
    if (axis == 0) report_.sThumbLX = value;
    else if (axis == 1) report_.sThumbLY = value;
    else if (axis == 2) report_.sThumbRX = value;
    else if (axis == 3) report_.sThumbRY = value;
    else {
        // XInput has two independent triggers, not a combined Z axis.
        // Preserve the signed router output by assigning each direction to one trigger.
        const BYTE negativeTrigger = static_cast<BYTE>(std::lround(std::max(0.0, -clampedFraction) * 255.0));
        const BYTE positiveTrigger = static_cast<BYTE>(std::lround(std::max(0.0, clampedFraction) * 255.0));
        report_.bLeftTrigger = flipTriggers ? positiveTrigger : negativeTrigger;
        report_.bRightTrigger = flipTriggers ? negativeTrigger : positiveTrigger;
    }
    vigem_target_x360_update(static_cast<PVIGEM_CLIENT>(client_), static_cast<PVIGEM_TARGET>(target_), report_);
}

void XboxOutput::shutdown() {
    if (client_ && target_) vigem_target_remove(static_cast<PVIGEM_CLIENT>(client_), static_cast<PVIGEM_TARGET>(target_));
    if (target_) vigem_target_free(static_cast<PVIGEM_TARGET>(target_));
    if (client_) { vigem_disconnect(static_cast<PVIGEM_CLIENT>(client_)); vigem_free(static_cast<PVIGEM_CLIENT>(client_)); }
    module_ = client_ = target_ = nullptr;
    report_ = {};
    active_ = false;
    status_ = L"Xbox output disabled";
}

// driver_multitron.dll - SteamVR driver exposing a virtual controller whose
// joystick/trigger/buttons are fed by MultiTron2.exe over shared memory.
//
// Runs inside vrserver.exe, so it stays small: no UI, no Interception, no
// Ultralight. All input processing remains in the MultiTron app.
#include <openvr_driver.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <thread>

#include "MultiTronIpc.h"

namespace {

// Role is the first thing to test in SteamVR. OptOut keeps the device from
// claiming a hand slot; if a game won't bind it, try LeftHand/RightHand.
constexpr vr::ETrackedControllerRole kRoleHint = vr::TrackedControllerRole_OptOut;

vr::DriverPose_t MakePose(bool connected) {
    vr::DriverPose_t pose = {};
    pose.qWorldFromDriverRotation.w = 1.0;
    pose.qDriverFromHeadRotation.w = 1.0;
    pose.qRotation.w = 1.0;
    // Fixed spot in front of and below the head: a locomotion input device doesn't move.
    pose.vecPosition[0] = 0.0;
    pose.vecPosition[1] = 0.8;
    pose.vecPosition[2] = -0.3;
    pose.poseIsValid = connected;
    pose.deviceIsConnected = connected;
    pose.result = connected ? vr::TrackingResult_Running_OK : vr::TrackingResult_Uninitialized;
    return pose;
}

class Controller final : public vr::ITrackedDeviceServerDriver {
public:
    ~Controller() { Deactivate(); }

    vr::EVRInitError Activate(uint32_t objectId) override {
        id_ = objectId;
        // Create-or-open; works whether or not MultiTron2.exe is running yet.
        if (!map_.Open()) {
            vr::VRDriverLog()->Log("multitron: could not open shared memory; inputs stay neutral");
        }

        auto* props = vr::VRProperties();
        const vr::PropertyContainerHandle_t c = props->TrackedDeviceToPropertyContainer(id_);
        props->SetStringProperty(c, vr::Prop_ModelNumber_String, "MultiTron Controller");
        props->SetStringProperty(c, vr::Prop_ManufacturerName_String, "MultiTron");
        props->SetStringProperty(c, vr::Prop_ControllerType_String, "multitron_controller");
        props->SetStringProperty(c, vr::Prop_InputProfilePath_String,
                                 "{multitron}/input/multitron_profile.json");
        props->SetInt32Property(c, vr::Prop_ControllerRoleHint_Int32, kRoleHint);

        auto* input = vr::VRDriverInput();
        input->CreateScalarComponent(c, "/input/thumbstick/x", &joyX_, vr::VRScalarType_Absolute,
                                     vr::VRScalarUnits_NormalizedTwoSided);
        input->CreateScalarComponent(c, "/input/thumbstick/y", &joyY_, vr::VRScalarType_Absolute,
                                     vr::VRScalarUnits_NormalizedTwoSided);
        input->CreateBooleanComponent(c, "/input/thumbstick/click", &joyClick_);
        input->CreateScalarComponent(c, "/input/trigger/value", &trigger_,
                                     vr::VRScalarType_Absolute,
                                     vr::VRScalarUnits_NormalizedOneSided);
        input->CreateBooleanComponent(c, "/input/a/click", &buttonA_);
        input->CreateBooleanComponent(c, "/input/b/click", &buttonB_);
        input->CreateHapticComponent(c, "/output/haptic", &haptic_);

        running_ = true;
        thread_ = std::thread([this] { Loop(); });
        return vr::VRInitError_None;
    }

    void Deactivate() override {
        running_ = false;
        if (thread_.joinable()) thread_.join();
        id_ = vr::k_unTrackedDeviceIndexInvalid;
    }

    void EnterStandby() override {}
    void* GetComponent(const char*) override { return nullptr; }
    void DebugRequest(const char*, char* response, uint32_t size) override {
        if (size > 0) response[0] = '\0';
    }
    vr::DriverPose_t GetPose() override { return MakePose(live_); }

    // Called from the provider's RunFrame() on the vrserver thread.
    void OnHaptic(const vr::VREvent_HapticVibration_t& h) {
        if (h.componentHandle != haptic_) return;
        auto* s = map_.state();
        if (!s) return;
        s->hapticAmplitude.store(h.fAmplitude, std::memory_order_relaxed);
        s->hapticFrequency.store(h.fFrequency, std::memory_order_relaxed);
        s->hapticDuration.store(h.fDurationSeconds, std::memory_order_relaxed);
        s->hapticSeq.fetch_add(1, std::memory_order_release);
    }

private:
    void Loop() {
        using namespace std::chrono_literals;
        auto* input = vr::VRDriverInput();
        multitron_ipc::Snapshot prev{};
        bool first = true;

        while (running_) {
            multitron_ipc::Snapshot s{};
            bool live = false;
            if (auto* st = map_.state()) {
                st->driverLastSeenMs.store(GetTickCount64(), std::memory_order_relaxed);
                s = multitron_ipc::ReadSnapshot(*st);
                live = s.lastWriteMs != 0 &&
                       (GetTickCount64() - s.lastWriteMs) < multitron_ipc::kStaleMs;
            }
            if (!live) s = {};  // stale or app closed: neutral inputs
            live_ = live;

            if (first || s.axisX != prev.axisX) input->UpdateScalarComponent(joyX_, s.axisX, 0.0);
            if (first || s.axisY != prev.axisY) input->UpdateScalarComponent(joyY_, s.axisY, 0.0);
            if (first || s.trigger != prev.trigger)
                input->UpdateScalarComponent(trigger_, s.trigger, 0.0);
            if (first || s.buttons != prev.buttons) {
                input->UpdateBooleanComponent(
                    joyClick_, (s.buttons & multitron_ipc::kButtonJoystickClick) != 0, 0.0);
                input->UpdateBooleanComponent(buttonA_, (s.buttons & multitron_ipc::kButtonA) != 0,
                                              0.0);
                input->UpdateBooleanComponent(buttonB_, (s.buttons & multitron_ipc::kButtonB) != 0,
                                              0.0);
            }
            prev = s;
            first = false;

            vr::VRServerDriverHost()->TrackedDevicePoseUpdated(id_, MakePose(live),
                                                               sizeof(vr::DriverPose_t));
            std::this_thread::sleep_for(4ms);  // ~250 Hz
        }
    }

    uint32_t id_ = vr::k_unTrackedDeviceIndexInvalid;
    multitron_ipc::Mapping map_;
    std::atomic<bool> running_{false};
    std::atomic<bool> live_{false};
    std::thread thread_;

    vr::VRInputComponentHandle_t joyX_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t joyY_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t joyClick_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t trigger_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t buttonA_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t buttonB_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t haptic_ = vr::k_ulInvalidInputComponentHandle;
};

class Provider final : public vr::IServerTrackedDeviceProvider {
public:
    vr::EVRInitError Init(vr::IVRDriverContext* ctx) override {
        VR_INIT_SERVER_DRIVER_CONTEXT(ctx);
        controller_ = std::make_unique<Controller>();
        if (!vr::VRServerDriverHost()->TrackedDeviceAdded(
                "multitron_controller_0", vr::TrackedDeviceClass_Controller, controller_.get())) {
            return vr::VRInitError_Driver_Unknown;
        }
        return vr::VRInitError_None;
    }

    void Cleanup() override {
        controller_.reset();
        VR_CLEANUP_SERVER_DRIVER_CONTEXT();
    }

    const char* const* GetInterfaceVersions() override { return vr::k_InterfaceVersions; }

    void RunFrame() override {
        vr::VREvent_t ev;
        while (vr::VRServerDriverHost()->PollNextEvent(&ev, sizeof(ev))) {
            if (ev.eventType == vr::VREvent_Input_HapticVibration && controller_) {
                controller_->OnHaptic(ev.data.hapticVibration);
            }
        }
    }

    bool ShouldBlockStandbyMode() override { return false; }
    void EnterStandby() override {}
    void LeaveStandby() override {}

private:
    std::unique_ptr<Controller> controller_;
};

Provider g_provider;

}  // namespace

extern "C" __declspec(dllexport) void* HmdDriverFactory(const char* interfaceName,
                                                        int* returnCode) {
    if (std::strcmp(vr::IServerTrackedDeviceProvider_Version, interfaceName) == 0) {
        return &g_provider;
    }
    if (returnCode) *returnCode = vr::VRInitError_Init_InterfaceNotFound;
    return nullptr;
}

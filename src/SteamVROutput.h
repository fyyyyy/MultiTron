#pragma once

#include <cstdint>

#include "MultiTronIpc.h"

// Shared-memory output feeding two independent consumers:
//  * the SteamVR driver (virtual controller)       -> updateController()/clearController()
//  * the OpenXR layer (added to the left stick)    -> updateStick()/clearStick()
class SteamVROutput {
public:
    // Opens the shared memory. Safe to call before SteamVR / a game is running.
    bool start();

    // Zeroes both channels and closes the shared memory (shutdown).
    void stop();

    // Call every router tick even when values are unchanged: the write time is
    // the heartbeat consumers use to detect a stopped or crashed app.
    void updateController(float axisX, float axisY, float trigger = 0.f, uint32_t buttons = 0);
    void clearController();
    void updateStick(float x, float y);
    void clearStick();

    // Returns true once per haptic pulse SteamVR sent to the virtual controller.
    bool pollHaptic(multitron_ipc::HapticPulse& out);

    bool isReady() const { return map_.state() != nullptr; }
    bool driverConnected() const;  // SteamVR driver is running and polling
    bool layerConnected() const;   // an OpenXR game with the layer is running

private:
    bool ensureOpen();
    void publish();

    multitron_ipc::Mapping map_;
    multitron_ipc::Snapshot current_;
    uint32_t lastHapticSeq_ = 0;
};

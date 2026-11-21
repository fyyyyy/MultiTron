#include "SteamVROutput.h"

#include <algorithm>

namespace {
float Clamp(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }
}  // namespace

bool SteamVROutput::start() {
    if (!ensureOpen()) return false;
    lastHapticSeq_ = map_.state()->hapticSeq.load(std::memory_order_acquire);
    return true;
}

bool SteamVROutput::ensureOpen() { return map_.state() != nullptr || map_.Open(); }

void SteamVROutput::publish() {
    if (map_.state()) multitron_ipc::WriteSnapshot(*map_.state(), current_);
}

void SteamVROutput::stop() {
    current_ = multitron_ipc::Snapshot{};  // all zero, timestamps 0 -> consumers see "stale"
    publish();
    map_.Close();
}

void SteamVROutput::updateController(float axisX, float axisY, float trigger, uint32_t buttons) {
    if (!ensureOpen()) return;
    current_.axisX = Clamp(axisX, -1.f, 1.f);
    current_.axisY = Clamp(axisY, -1.f, 1.f);
    current_.trigger = Clamp(trigger, 0.f, 1.f);
    current_.buttons = buttons;
    current_.lastWriteMs = GetTickCount64();
    publish();
}

void SteamVROutput::clearController() {
    current_.axisX = current_.axisY = current_.trigger = 0.f;
    current_.buttons = 0;
    current_.lastWriteMs = 0;
    publish();
}

void SteamVROutput::updateStick(float x, float y) {
    if (!ensureOpen()) return;
    current_.stickX = Clamp(x, -1.f, 1.f);
    current_.stickY = Clamp(y, -1.f, 1.f);
    current_.stickWriteMs = GetTickCount64();
    publish();
}

void SteamVROutput::clearStick() {
    current_.stickX = current_.stickY = 0.f;
    current_.stickWriteMs = 0;
    publish();
}

bool SteamVROutput::driverConnected() const {
    if (!map_.state()) return false;
    const uint64_t seen = map_.state()->driverLastSeenMs.load(std::memory_order_relaxed);
    return seen != 0 && GetTickCount64() - seen < 1000;
}

bool SteamVROutput::layerConnected() const {
    if (!map_.state()) return false;
    const uint64_t seen = map_.state()->layerLastSeenMs.load(std::memory_order_relaxed);
    return seen != 0 && GetTickCount64() - seen < 1000;
}

bool SteamVROutput::pollHaptic(multitron_ipc::HapticPulse& out) {
    if (!map_.state()) return false;
    auto& st = *map_.state();
    const uint32_t seq = st.hapticSeq.load(std::memory_order_acquire);
    if (seq == lastHapticSeq_) return false;
    lastHapticSeq_ = seq;
    out.amplitude = st.hapticAmplitude.load(std::memory_order_relaxed);
    out.frequency = st.hapticFrequency.load(std::memory_order_relaxed);
    out.durationSeconds = st.hapticDuration.load(std::memory_order_relaxed);
    return true;
}

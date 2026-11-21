// Shared-memory protocol between MultiTron2.exe (writer) and driver_multitron.dll
// (reader, loaded inside vrserver.exe). Header-only, used by both sides.
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <cstdint>

namespace multitron_ipc {

inline constexpr wchar_t kMappingName[] = L"Local\\MultiTronSteamVR";
inline constexpr uint32_t kMagic = 0x4E525454;  // 'TTRN'
inline constexpr uint32_t kVersion = 1;

// If the app stops writing for this long, the driver treats it as disconnected
// and zeroes all inputs (protects against a crashed app leaving the stick pinned).
inline constexpr uint64_t kStaleMs = 500;

enum Buttons : uint32_t {
    kButtonA = 1u << 0,
    kButtonB = 1u << 1,
    kButtonJoystickClick = 1u << 2,
};

struct Snapshot {
    float axisX = 0.f;     // -1..1
    float axisY = 0.f;     // -1..1 (forward = +1)
    float trigger = 0.f;   // 0..1
    uint32_t buttons = 0;  // Buttons bitmask
    uint64_t lastWriteMs = 0;   // virtual controller (SteamVR driver) channel

    float stickX = 0.f;         // OpenXR layer channel: added to the real left thumbstick
    float stickY = 0.f;
    uint64_t stickWriteMs = 0;
};

struct HapticPulse {
    float amplitude = 0.f;
    float frequency = 0.f;
    float durationSeconds = 0.f;
};

struct SharedState {
    std::atomic<uint32_t> magic;
    std::atomic<uint32_t> version;

    // app -> driver (seqlock: odd while the app is mid-write)
    std::atomic<uint32_t> seq;
    std::atomic<float> axisX;
    std::atomic<float> axisY;
    std::atomic<float> trigger;
    std::atomic<uint32_t> buttons;
    std::atomic<uint64_t> lastWriteMs;
    std::atomic<float> stickX;
    std::atomic<float> stickY;
    std::atomic<uint64_t> stickWriteMs;

    // driver -> app: GetTickCount64() stamped every driver tick while SteamVR runs
    std::atomic<uint64_t> driverLastSeenMs;
    // OpenXR layer -> app: GetTickCount64() stamped whenever the layer polls
    std::atomic<uint64_t> layerLastSeenMs;

    // driver -> app (hapticSeq increments after the fields are written)
    std::atomic<uint32_t> hapticSeq;
    std::atomic<float> hapticAmplitude;
    std::atomic<float> hapticFrequency;
    std::atomic<float> hapticDuration;
};

static_assert(std::atomic<uint32_t>::is_always_lock_free && std::atomic<uint64_t>::is_always_lock_free
                  && std::atomic<float>::is_always_lock_free,
              "atomics in shared memory must be lock-free");

// Single writer (app) only.
inline void WriteSnapshot(SharedState& s, const Snapshot& in) {
    s.seq.fetch_add(1, std::memory_order_release);  // now odd
    s.axisX.store(in.axisX, std::memory_order_relaxed);
    s.axisY.store(in.axisY, std::memory_order_relaxed);
    s.trigger.store(in.trigger, std::memory_order_relaxed);
    s.buttons.store(in.buttons, std::memory_order_relaxed);
    s.lastWriteMs.store(in.lastWriteMs, std::memory_order_relaxed);
    s.stickX.store(in.stickX, std::memory_order_relaxed);
    s.stickY.store(in.stickY, std::memory_order_relaxed);
    s.stickWriteMs.store(in.stickWriteMs, std::memory_order_relaxed);
    s.seq.fetch_add(1, std::memory_order_release);  // even again
}

inline Snapshot ReadSnapshot(SharedState& s) {
    Snapshot out;
    for (int attempt = 0; attempt < 8; ++attempt) {
        const uint32_t before = s.seq.load(std::memory_order_acquire);
        if (before & 1u) continue;
        out.axisX = s.axisX.load(std::memory_order_relaxed);
        out.axisY = s.axisY.load(std::memory_order_relaxed);
        out.trigger = s.trigger.load(std::memory_order_relaxed);
        out.buttons = s.buttons.load(std::memory_order_relaxed);
        out.lastWriteMs = s.lastWriteMs.load(std::memory_order_relaxed);
        out.stickX = s.stickX.load(std::memory_order_relaxed);
        out.stickY = s.stickY.load(std::memory_order_relaxed);
        out.stickWriteMs = s.stickWriteMs.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (s.seq.load(std::memory_order_relaxed) == before) return out;
    }
    return Snapshot{};  // couldn't get a stable read; treat as neutral
}

// Create-or-open, so it doesn't matter whether the app or SteamVR starts first.
class Mapping {
public:
    Mapping() = default;
    ~Mapping() { Close(); }
    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;

    bool Open() {
        if (state_) return true;
        handle_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                                     static_cast<DWORD>(sizeof(SharedState)), kMappingName);
        if (!handle_) return false;
        void* view = MapViewOfFile(handle_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedState));
        if (!view) {
            CloseHandle(handle_);
            handle_ = nullptr;
            return false;
        }
        state_ = static_cast<SharedState*>(view);
        uint32_t expected = 0;
        if (state_->magic.compare_exchange_strong(expected, kMagic)) {
            state_->version.store(kVersion);
        }
        return true;
    }

    void Close() {
        if (state_) UnmapViewOfFile(state_);
        if (handle_) CloseHandle(handle_);
        state_ = nullptr;
        handle_ = nullptr;
    }

    SharedState* state() const { return state_; }

private:
    HANDLE handle_ = nullptr;
    SharedState* state_ = nullptr;
};

}  // namespace multitron_ipc

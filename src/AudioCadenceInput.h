#pragma once

#include <algorithm>
#include <atomic>
#include <array>
#include <vector>
#include <windows.h>
#include <mmsystem.h>

// Captures the Windows default recording device and treats a distinct audio
// pulse as one pedal revolution.  This matches the common 3.5 mm cadence
// sensor output used by older exercise bikes.
class AudioCadenceInput {
public:
    static constexpr int kMinimumPulseAmplitude = 1;
    static constexpr int kMaximumPulseAmplitude = 255;
    // A pulse can legitimately be several seconds apart at low cadence. Keep
    // the last valid value until the same timeout used by the router's audio
    // freshness check, avoiding brief zero-RPM dropouts from one missed pulse.
    static constexpr ULONGLONG kRpmStaleMs = 6500;
    ~AudioCadenceInput();

    bool start();
    void stop();
    bool active() const { return active_.load(); }
    double rpm() const {
        const ULONGLONG pulse = lastPulseTick_.load();
        const double currentRpm = rpm_.load();
        // A fast cadence should not remain active for the six-second window
        // intended for very slow pedalling. Allow roughly two expected pulse
        // intervals, with a 1.5s floor for small timing jitter.
        const ULONGLONG freshnessMs = currentRpm > 0.0 ? std::min(kRpmStaleMs,
            std::max<ULONGLONG>(1500, static_cast<ULONGLONG>(120000.0 / currentRpm))) : 0;
        return pulse != 0 && GetTickCount64() - pulse <= freshnessMs ? currentRpm : 0.0;
    }
    ULONGLONG lastPulseTick() const { return lastPulseTick_.load(); }
    int peakAmplitude() const { return peakAmplitude_.load(); }
    int pulseAmplitude() const { return pulseAmplitude_.load(); }
    void setPulseAmplitude(int value);
    unsigned long pulseSequence() const { return pulseSequence_.load(); }
    void setMouseActivity(long activity) { mouseActivity_ = activity < 0 ? -activity : activity; }
    void resetRpm();

private:
    static void CALLBACK callback(HWAVEIN input, UINT message, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2);
    void process(WAVEHDR* header);

    static constexpr int kSampleRate = 44100;
    // The 3.5 mm sensor is a very low-level source on a microphone input.
    // Keep this just above the observed idle noise floor (~20).
    ULONGLONG minimumPulseGapMs() const;
    // 10 RPM is one pedal pulse every six seconds.
    static constexpr ULONGLONG kMaximumPulseGapMs = 6500;
    HWAVEIN input_ = nullptr;
    std::array<WAVEHDR, 4> headers_{};
    std::array<std::vector<short>, 4> buffers_;
    std::atomic<bool> active_{false};
    std::atomic<bool> signalHigh_{false};
    std::atomic<double> rpm_{0.0};
    std::atomic<ULONGLONG> lastPulseTick_{0};
    std::atomic<int> peakAmplitude_{0};
    std::atomic<int> pulseAmplitude_{30};
    std::atomic<unsigned long> pulseSequence_{0};
    std::atomic<long> mouseActivity_{0};
};

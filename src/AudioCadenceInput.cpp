#include "AudioCadenceInput.h"

#include <algorithm>
#include <cmath>

AudioCadenceInput::~AudioCadenceInput() {
    stop();
}

void AudioCadenceInput::resetRpm() {
    rpm_ = 0.0;
    lastPulseTick_ = 0;
}

void AudioCadenceInput::setPulseAmplitude(int value) {
    pulseAmplitude_ = std::clamp(value, kMinimumPulseAmplitude, kMaximumPulseAmplitude);
}

ULONGLONG AudioCadenceInput::minimumPulseGapMs() const {
    // Mouse activity is only a stop signal for the output router. It must not
    // alter audio pulse detection: at 75 RPM, valid pulses are about 800 ms
    // apart and were previously rejected by the 900 ms idle-mouse gate.
    return 450;
}

bool AudioCadenceInput::start() {
    if (active_.load()) return true;
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = kSampleRate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    if (waveInOpen(&input_, WAVE_MAPPER, &format, reinterpret_cast<DWORD_PTR>(&callback),
                   reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        input_ = nullptr;
        return false;
    }

    for (size_t index = 0; index < headers_.size(); ++index) {
        buffers_[index].resize(2048);
        auto& header = headers_[index];
        header.lpData = reinterpret_cast<LPSTR>(buffers_[index].data());
        header.dwBufferLength = static_cast<DWORD>(buffers_[index].size() * sizeof(short));
        if (waveInPrepareHeader(input_, &header, sizeof(header)) != MMSYSERR_NOERROR
            || waveInAddBuffer(input_, &header, sizeof(header)) != MMSYSERR_NOERROR) {
            stop();
            return false;
        }
    }
    signalHigh_ = false;
    rpm_ = 0.0;
    lastPulseTick_ = 0;
    peakAmplitude_ = 0;
    pulseSequence_ = 0;
    active_ = waveInStart(input_) == MMSYSERR_NOERROR;
    if (!active_.load()) stop();
    return active_.load();
}

void AudioCadenceInput::stop() {
    active_ = false;
    if (!input_) return;
    waveInStop(input_);
    waveInReset(input_);
    for (auto& header : headers_) {
        if (header.dwFlags & WHDR_PREPARED) waveInUnprepareHeader(input_, &header, sizeof(header));
    }
    waveInClose(input_);
    input_ = nullptr;
    rpm_ = 0.0;
    lastPulseTick_ = 0;
    peakAmplitude_ = 0;
    pulseSequence_ = 0;
}

void CALLBACK AudioCadenceInput::callback(HWAVEIN, UINT message, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR) {
    if (message != WIM_DATA || !instance) return;
    static_cast<AudioCadenceInput*>(reinterpret_cast<void*>(instance))->process(reinterpret_cast<WAVEHDR*>(param1));
}

void AudioCadenceInput::process(WAVEHDR* header) {
    if (!active_.load() || !header) return;
    const auto* samples = reinterpret_cast<const short*>(header->lpData);
    const size_t count = header->dwBytesRecorded / sizeof(short);
    bool high = signalHigh_.load();
    const int threshold = pulseAmplitude_.load();
    int peak = 0;
    for (size_t index = 0; index < count; ++index) {
        const int amplitude = std::abs(static_cast<int>(samples[index]));
        peak = std::max(peak, amplitude);
        if (!high && amplitude >= threshold) {
            const ULONGLONG now = GetTickCount64();
            const ULONGLONG previous = lastPulseTick_.load();
            if (previous == 0) {
                lastPulseTick_ = now;
                ++pulseSequence_;
            } else {
                const ULONGLONG gap = now - previous;
                if (gap >= minimumPulseGapMs() && gap <= kMaximumPulseGapMs) {
                    rpm_ = 60000.0 / static_cast<double>(gap);
                    lastPulseTick_ = now;
                    ++pulseSequence_;
                } else if (gap > kMaximumPulseGapMs) {
                    // Start a fresh interval after a long stop.
                    rpm_ = 0.0;
                    lastPulseTick_ = now;
                    ++pulseSequence_;
                }
            }
            high = true;
        } else if (high && amplitude < (std::max)(1, threshold / 2)) {
            high = false;
        }
    }
    signalHigh_ = high;
    peakAmplitude_ = peak;
    if (active_.load() && input_) waveInAddBuffer(input_, header, sizeof(*header));
}

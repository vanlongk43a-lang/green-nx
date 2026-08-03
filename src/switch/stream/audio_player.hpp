#pragma once

#include <switch.h>

#include <atomic>
#include <cstdint>

namespace gnx::stream {

class AudioPlayer {
public:
    bool init();
    void shutdown();

    void submit(uint16_t seq, const uint8_t* data, size_t size);

    int device_hz() const { return 48000; }
    void set_gain(float gain) { (void)gain; }

    struct Stats {
        uint32_t received = 0;
        uint32_t played = 0;
        uint32_t failed = 0;
        uint32_t lost = 0;
        uint32_t underruns = 0;
        uint32_t dropped_ms = 0;
        uint32_t queue_ms = 0;
        uint32_t frames = 0;
        uint32_t out_samples = 0;
        uint32_t ema_ms = 0;
        int32_t adj_ppm = 0;
    };
    Stats stats() const;
};

}  // namespace gnx::stream

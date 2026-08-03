#include "audio_player.hpp"

namespace gnx::stream {

bool AudioPlayer::init() {
    return true;
}

void AudioPlayer::shutdown() {
}

void AudioPlayer::submit(uint16_t seq, const uint8_t* data, size_t size) {
    (void)seq;
    (void)data;
    (void)size;
}

AudioPlayer::Stats AudioPlayer::stats() const {
    return Stats{};
}

}  // namespace gnx::stream

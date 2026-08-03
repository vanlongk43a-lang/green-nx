#include "audio_player.hpp"

namespace gnx::stream {

// Khởi tạo Audio Player ảo: Luôn trả về true để không làm ngắt kết nối
bool AudioPlayer::init() {
    return true;
}

// Tắt Audio Player: Không cần giải phóng thiết bị âm thanh
void AudioPlayer::shutdown() {
}

// Bỏ qua toàn bộ gói tin âm thanh truyền về từ Xbox
void AudioPlayer::play(const uint8_t* data, size_t size) {
    (void)data;
    (void)size;
}

}  // namespace gnx::stream

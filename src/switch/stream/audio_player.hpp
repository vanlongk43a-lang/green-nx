#include "audio_player.hpp"

namespace gnx::stream {

// Khởi tạo Audio Player giả lập: Trả về true ngay để app tiếp tục chạy
bool AudioPlayer::init() {
    return true;
}

// Tắt Audio Player: Không mở luồng phát nhạc nên không cần dọn dẹp
void AudioPlayer::shutdown() {
}

// Bỏ qua hoàn toàn các gói tin âm thanh truyền về từ Xbox
void AudioPlayer::submit(uint16_t seq, const uint8_t* data, size_t size) {
    (void)seq;
    (void)data;
    (void)size;
}

// Báo cáo thống kê rỗng để tránh lỗi gọi hàm telemetry
AudioPlayer::Stats AudioPlayer::stats() const {
    return Stats{};
}

}  // namespace gnx::stream

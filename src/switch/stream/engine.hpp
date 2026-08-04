#pragma once

#include <switch.h>
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>

// WebRTC Interfaces
#include <api/peer_connection_interface.h>
#include <api/data_channel_interface.h>

// Bỏ include dk_video_renderer.hpp và audio_player.hpp để tối ưu build

// Định nghĩa mã nút bấm Xbox
enum XboxButtons : uint16_t {
    XBOX_BTN_A          = 1 << 0,
    XBOX_BTN_B          = 1 << 1,
    XBOX_BTN_X          = 1 << 2,
    XBOX_BTN_Y          = 1 << 3,
    XBOX_BTN_LB         = 1 << 4,
    XBOX_BTN_RB         = 1 << 5,
    XBOX_BTN_LT         = 1 << 6,
    XBOX_BTN_RT         = 1 << 7,
    XBOX_BTN_SELECT     = 1 << 8,
    XBOX_BTN_START      = 1 << 9,
    XBOX_BTN_THUMB_L    = 1 << 10,
    XBOX_BTN_THUMB_R    = 1 << 11,
    XBOX_BTN_DPAD_UP    = 1 << 12,
    XBOX_BTN_DPAD_DOWN  = 1 << 13,
    XBOX_BTN_DPAD_LEFT  = 1 << 14,
    XBOX_BTN_DPAD_RIGHT = 1 << 15,
};

// Struct chứa gói tin phím bấm gửi sang Xbox
#pragma pack(push, 1)
struct XboxInputReport {
    uint16_t buttons;
    int16_t  left_stick_x;
    int16_t  left_stick_y;
    int16_t  right_stick_x;
    int16_t  right_stick_y;
};
#pragma pack(pop)

class Engine : public webrtc::DataChannelObserver,
               public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
    Engine();
    ~Engine();

    bool initialize();
    bool start();
    void stop();

    bool create_peer_connection();
    void configure_sdp_controller_only(webrtc::SessionDescriptionInterface* sdp);

    // DataChannelObserver Overrides
    void OnStateChange() override;
    void OnMessage(const webrtc::DataBuffer& buffer) override;

    // VideoSinkInterface Overrides (Để trống trong Controller Mode)
    void OnFrame(const webrtc::VideoFrame& frame) override;

    // Audio Data Callback (Để trống)
    void OnAudioData(const void* audio_data, int bits_per_sample, int sample_rate, 
                     size_t number_of_channels, size_t number_of_frames);

private:
    bool setup_data_channels();
    void input_loop();
    uint16_t map_switch_buttons_to_xbox(u64 keys_held);
    void handle_rumble_event(const uint8_t* data, size_t size);

private:
    std::atomic<bool> m_running;
    std::atomic<bool> m_connected;

    // Libnx Pad State
    PadState m_pad_state;

    // WebRTC Components
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> m_peer_factory;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface>        m_peer_connection;
    rtc::scoped_refptr<webrtc::DataChannelInterface>           m_input_channel;
    rtc::scoped_refptr<webrtc::DataChannelInterface>           m_control_channel;

    // Luồng quét phím
    std::thread m_input_thread;
};

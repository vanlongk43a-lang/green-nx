#include "engine.hpp"
#include <switch.h>
#include <chrono>
#include <thread>
#include <iostream>

// LƯU Ý: Đã tắt kết nối tới dk_video_renderer.hpp và audio_player.hpp
// để phục vụ chế độ Controller-Only (tiết kiệm pin & tối ưu latency).

Engine::Engine() 
    : m_running(false)
    , m_connected(false)
    , m_peer_connection(nullptr)
    , m_input_channel(nullptr)
    , m_control_channel(nullptr) {
    
    // Khởi tạo hệ thống Pad (Input) của libnx
    padConfigureInput(1, HIDNPAD_MODE_HANDHELD_CONS);
    padInitializeDefault(&m_pad_state);
}

Engine::~Engine() {
    stop();
}

bool Engine::initialize() {
    std::cout << "[Engine] Controller-Only Mode Initialized." << std::endl;
    // Bỏ qua khởi tạo AudioPlayer và DkVideoRenderer
    return true;
}

bool Engine::create_peer_connection() {
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    // Cấu hình STUN/TURN server nếu cần thiết
    
    webrtc::PeerConnectionDependencies dependencies(this);
    auto result = m_peer_factory->CreatePeerConnectionOrError(config, std::move(dependencies));
    
    if (!result.ok()) {
        std::cerr << "[Engine] Failed to create PeerConnection!" << std::endl;
        return false;
    }
    
    m_peer_connection = result.MoveValue();
    return setup_data_channels();
}

bool Engine::setup_data_channels() {
    if (!m_peer_connection) return false;

    // 1. Tạo Input Data Channel để gửi phím bấm về Xbox
    webrtc::DataChannelInit input_config;
    input_config.ordered = true;
    input_config.maxRetransmits = 0; // Tối ưu latency, bỏ qua gói tin bị lỡ

    auto input_result = m_peer_connection->CreateDataChannelOrError("input", &input_config);
    if (input_result.ok()) {
        m_input_channel = input_result.MoveValue();
        m_input_channel->RegisterObserver(this);
        std::cout << "[Engine] Input Data Channel created." << std::endl;
    } else {
        std::cerr << "[Engine] Failed to create Input Data Channel!" << std::endl;
        return false;
    }

    // 2. Tạo Control Data Channel để nhận Rung (Rumble) / Telemetry từ Xbox
    webrtc::DataChannelInit control_config;
    control_config.ordered = true;
    auto control_result = m_peer_connection->CreateDataChannelOrError("control", &control_config);
    if (control_result.ok()) {
        m_control_channel = control_result.MoveValue();
        m_control_channel->RegisterObserver(this);
        std::cout << "[Engine] Control Data Channel created." << std::endl;
    }

    return true;
}

void Engine::configure_sdp_controller_only(webrtc::SessionDescriptionInterface* sdp) {
    // Tắt tất cả các Transceiver âm thanh và hình ảnh trong SDP
    if (!m_peer_connection) return;

    for (auto transceiver : m_peer_connection->GetTransceivers()) {
        auto media_type = transceiver->media_type();
        if (media_type == cricket::MEDIA_TYPE_AUDIO || media_type == cricket::MEDIA_TYPE_VIDEO) {
            // Yêu cầu Xbox không gửi/nhận dữ liệu Stream Video/Audio
            transceiver->SetDirectionWithError(webrtc::RtpTransceiverDirection::kInactive);
            std::cout << "[Engine] Media transceiver set to kInactive (Disabled)." << std::endl;
        }
    }
}

bool Engine::start() {
    if (m_running) return true;

    m_running = true;
    m_connected = true;

    // Khởi chạy luồng đọc & gửi phím bấm (Input Loop)
    m_input_thread = std::thread(&Engine::input_loop, this);

    std::cout << "[Engine] Controller Mode Started. Streaming inputs to Xbox..." << std::endl;
    return true;
}

void Engine::stop() {
    if (!m_running) return;

    m_running = false;
    m_connected = false;

    if (m_input_thread.joinable()) {
        m_input_thread.join();
    }

    if (m_peer_connection) {
        m_peer_connection->Close();
        m_peer_connection = nullptr;
    }

    std::cout << "[Engine] Controller Mode Stopped." << std::endl;
}

void Engine::input_loop() {
    // Vòng lặp gửi phím bấm chạy ở tần số ~120Hz (~8.3ms) để giảm tối đa độ trễ
    constexpr auto kTickRate = std::chrono::microseconds(8333);

    while (m_running) {
        auto start_time = std::chrono::steady_clock::now();

        // 1. Cập nhật trạng thái nút bấm từ Nintendo Switch (libnx)
        padUpdate(&m_pad_state);
        u64 keys_held = padGetButtons(&m_pad_state);
        
        HidAnalogStickState stick_left = padGetStickPos(&m_pad_state, 0);
        HidAnalogStickState stick_right = padGetStickPos(&m_pad_state, 1);

        // 2. Map dữ liệu từ Switch sang định dạng Xbox Report Packet
        XboxInputReport report = {};
        report.buttons = map_switch_buttons_to_xbox(keys_held);
        report.left_stick_x = stick_left.x;
        report.left_stick_y = stick_left.y;
        report.right_stick_x = stick_right.x;
        report.right_stick_y = stick_right.y;

        // 3. Gửi gói tin qua WebRTC Input Channel
        if (m_input_channel && m_input_channel->state() == webrtc::DataChannelInterface::kOpen) {
            rtc::CopyOnWriteBuffer buffer(reinterpret_cast<const uint8_t*>(&report), sizeof(report));
            m_input_channel->Send(webrtc::DataBuffer(buffer, true));
        }

        // Chờ đến chu kỳ tiếp theo
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed < kTickRate) {
            std::this_thread::sleep_for(kTickRate - elapsed);
        }
    }
}

uint16_t Engine::map_switch_buttons_to_xbox(u64 keys_held) {
    uint16_t xbox_buttons = 0;

    // Chuyển đổi nút bấm từ Switch -> Xbox (Đã đảo A/B, X/Y cho đúng vị trí vật lý)
    if (keys_held & HydraNpadButton_B)      xbox_buttons |= XBOX_BTN_A;
    if (keys_held & HydraNpadButton_A)      xbox_buttons |= XBOX_BTN_B;
    if (keys_held & HydraNpadButton_Y)      xbox_buttons |= XBOX_BTN_X;
    if (keys_held & HydraNpadButton_X)      xbox_buttons |= XBOX_BTN_Y;
    
    if (keys_held & HydraNpadButton_Minus)  xbox_buttons |= XBOX_BTN_SELECT;
    if (keys_held & HydraNpadButton_Plus)   xbox_buttons |= XBOX_BTN_START;
    
    if (keys_held & HydraNpadButton_ZL)     xbox_buttons |= XBOX_BTN_LT;
    if (keys_held & HydraNpadButton_ZR)     xbox_buttons |= XBOX_BTN_RT;
    if (keys_held & HydraNpadButton_L)      xbox_buttons |= XBOX_BTN_LB;
    if (keys_held & HydraNpadButton_R)      xbox_buttons |= XBOX_BTN_RB;

    if (keys_held & HydraNpadButton_StickL) xbox_buttons |= XBOX_BTN_THUMB_L;
    if (keys_held & HydraNpadButton_StickR) xbox_buttons |= XBOX_BTN_THUMB_R;

    if (keys_held & HydraNpadButton_Up)     xbox_buttons |= XBOX_BTN_DPAD_UP;
    if (keys_held & HydraNpadButton_Down)   xbox_buttons |= XBOX_BTN_DPAD_DOWN;
    if (keys_held & HydraNpadButton_Left)   xbox_buttons |= XBOX_BTN_DPAD_LEFT;
    if (keys_held & HydraNpadButton_Right)  xbox_buttons |= XBOX_BTN_DPAD_RIGHT;

    return xbox_buttons;
}

// --- WEBRTC CALLBACKS ---

void Engine::OnMessage(const webrtc::DataBuffer& buffer) {
    // Xử lý gói tin phản hồi từ Xbox (ví dụ: tín hiệu rung Rumble)
    if (buffer.size() > 0) {
        handle_rumble_event(buffer.data.data(), buffer.size());
    }
}

void Engine::OnStateChange() {
    if (m_input_channel) {
        std::cout << "[Engine] Input Channel State Changed: " 
                  << m_input_channel->state() << std::endl;
    }
}

void Engine::handle_rumble_event(const uint8_t* data, size_t size) {
    // Xử lý kích hoạt mô-tơ rung của Joy-Con / Pro Controller qua libnx (nếu có)
}

// Bỏ qua tất cả các luồng nhận Frame Video và Audio
void Engine::OnFrame(const webrtc::VideoFrame& frame) { return; }
void Engine::OnAudioData(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) { return; }

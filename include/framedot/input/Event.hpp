// include/framedot/input/Event.hpp
#pragma once
#include <cstdint>
#include <framedot/input/Key.hpp>


namespace framedot::input {

    /// @brief 입력 이벤트 타입.
    enum class EventType : std::uint8_t {
        None = 0,
        Key,
        MouseMove,
        MouseButton,
        MouseWheel,
        Text,        // UTF-8 텍스트 입력(나중에)
    };

    /// @brief 키 이벤트 페이로드
    struct KeyEvent {
        Key         key;
        KeyAction   action;
    };

    /// @brief 마우스 이동
    struct MouseMoveEvent {
        std::int32_t x;
        std::int32_t y;
    };

    /// @brief 마우스 버튼
    struct MouseButtonEvent {
        std::int32_t button;
        std::int32_t down;   // 1=down, 0=up
    };

    /// @brief 입력 이벤트 
    struct Event {
        EventType type;

        union {
            KeyEvent         key;
            MouseMoveEvent   mouse_move;
            MouseButtonEvent mouse_btn;
        } data;
    };

} // namespace framedot::input
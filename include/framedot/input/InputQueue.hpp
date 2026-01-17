#pragma once
#include <cstddef>
#include <array>
#include <span>

#include <framedot/input/Event.hpp>
#include <framedot/core/Config.hpp>


namespace framedot::input {

    /// @brief 프레임 단위로 입력 이벤트를 모아두는 고정 용량 큐.
    /// - 동적 할당을 하지 않아서 플랫폼/OS 독립성에 유리하다.
    /// - 용량은 framedot::core::config::max_input_events 로 제어된다(CMake로 조절).
    class InputQueue {
    public:
        /// @brief 큐를 비운다(프레임 시작 시 호출).
        void clear() noexcept { m_size = 0; }

        /// @brief 이벤트를 큐에 추가한다.
        /// @return 성공하면 true, 큐가 가득 차서 드랍되면 false.
        bool push(const Event& ev) noexcept {
            /// @brief 입력 폭주 상황에서는 안전하게 드랍한다(게임 루프가 멈추면 안 됨).
            if (m_size >= capacity()) return false;
            m_events[m_size++] = ev;
            return true;
        }

        /// @brief 이벤트들을 span으로 조회한다.
        std::span<const Event> events() const noexcept {
            return std::span<const Event>(m_events.data(), m_size);
        }

        /// @brief 이벤트 개수.
        std::size_t size() const noexcept { return m_size; }

        /// @brief 최대 용량.
        static constexpr std::size_t capacity() noexcept {
            return framedot::core::config::max_input_events;
        }

        /// @brief 가득 찼는지 여부.
        bool full() const noexcept { return m_size >= capacity(); }

    private:
        std::size_t m_size{0};
        std::array<Event, framedot::core::config::max_input_events> m_events{};
    };

} // namespace framedot::input
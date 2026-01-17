#pragma once
#include <cstddef>
#include <cstdint>
#include <array>
#include <span>

#include <framedot/input/Event.hpp>
#include <framedot/core/Config.hpp>


namespace framedot::input {

    /// @brief 입력 큐 오버플로 정책
    enum class OverflowPolicy : std::uint32_t {
        DropNewest = 0,
        DropOldest = 1,
        CoalesceMouseMove = 2, // 예약: 마우스 이동 합치기 (고빈도 최적화)
    };

    /// @brief 프레임 단위 입력 이벤트를 모아두는 고정 용량 큐.
    /// - 동적 할당 없이 동작한다.
    /// - 오버플로 발생 시 정책에 따라 이벤트 기록을 드랍한다.
    class InputQueue {
    public:
        /// @brief 프레임 시작 시 큐를 flush
        void clear() noexcept {
            m_size = 0;
            m_dropped = 0;
        }

        /// @brief 현재 프레임에서 드랍된 이벤트 개수
        std::size_t dropped() const noexcept { return m_dropped; }

        /// @brief 현재 저장된 이벤트 개수
        std::size_t size() const noexcept { return m_size; }

        /// @brief 최대 용량
        static constexpr std::size_t capacity() noexcept {
            return framedot::core::config::max_input_events;
        }

        /// @brief 이벤트들을 span으로 조회
        std::span<const Event> events() const noexcept {
            return std::span<const Event>(m_events.data(), m_size);
        }

        /// @brief 이벤트를 큐에 기록한다.
        /// @return 기록 성공 true, DropNewest 정책으로 인해 기록 실패 false.
        bool push(const Event& ev) noexcept {
            if (m_size < capacity()) {
                m_events[m_size++] = ev;
                return true;
            }

            const auto pol = static_cast<OverflowPolicy>(framedot::core::config::input_overflow_policy);

            if (pol == OverflowPolicy::DropNewest) {
                /// @brief 새 이벤트 기록을 버리기 (상태는 InputState가 따로 보존)
                ++m_dropped;
                return false;
            }

            if (pol == OverflowPolicy::DropOldest) {
                /// @brief 가장 오래된 이벤트를 버리고, 최신 이벤트를 끝에 유지
                /// 용량이 작고(기본 256) 입력 폭주가 드물기 때문에 O(N) 쉬프트 허용
                for (std::size_t i = 1; i < m_size; ++i) {
                    m_events[i - 1] = m_events[i];
                }
                m_events[m_size - 1] = ev;
                ++m_dropped;
                return true;
            }

            /// @brief CoalesceMouseMove는 현재 예약. 우선 DropNewest처럼 처리.
            ++m_dropped;
            return false;
        }

    private:
        std::size_t m_size{0};
        std::size_t m_dropped{0};
        std::array<Event, framedot::core::config::max_input_events> m_events{};
    };

} // namespace framedot::input
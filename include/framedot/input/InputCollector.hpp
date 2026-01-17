// include/framedot/input/InputCollector.hpp
#pragma once
#include <framedot/input/InputQueue.hpp>
#include <framedot/input/InputState.hpp>


namespace framedot::input {

    /// @brief 플랫폼 입력을 받아 InputState/Queue로 분배하는 수집기
    /// - 상태(InputState)는 항상 갱신
    /// - 큐(InputQueue)는 오버플로 정책에 따라 기록의 드랍 가능
    class InputCollector {
    public:
        /// @brief 상태/큐 참조로 생성
        InputCollector(InputState& state, InputQueue& queue) noexcept
            : m_state(state), m_queue(queue) {}

        /// @brief 이벤트를 입력 시스템에 주입
        /// @return 큐 기록 성공 여부 (상태 갱신은 항상 수행)
        bool push(const Event& ev) noexcept {
            m_state.apply(ev);
            return m_queue.push(ev);
        }

        /// @brief 상태 접근
        InputState& state() noexcept {  return m_state;  }

        /// @brief 큐 접근
        InputQueue& queue() noexcept {  return m_queue;  }


    private:
        InputState& m_state;
        InputQueue& m_queue;
    };

} // namespace framedot::input
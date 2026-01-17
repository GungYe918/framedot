// include/framedot/core/FrameContext.hpp
/**
 * @file FrameContext.hpp
 * @brief 한 프레임 동안 불변으로 취급하는 스냅샷 컨텍스트.
 *
 * 멀티스레드 업데이트/RenderPrep를 위해, "읽기 쉬운 포인터 뭉치" 형태를 유지한다.
 */
#pragma once
#include <framedot/input/InputQueue.hpp>
#include <framedot/input/InputState.hpp>
#include <framedot/core/JobSystem.hpp>
#include <framedot/gfx/RenderQueue.hpp>

#include <cstdint>


namespace framedot::core {

    /// @brief 한 프레임 동안 변하지 않는 프레임 스냅샷 컨텍스트
    /// - 멀티스레드 업데이트를 염두에 두고, 읽기 전용으로 전달하기 쉬운 형태 목표
    struct FrameContext {
        /// @brief 프레임 인덱스 (0부터 증가)
        std::uint64_t   frame_index  {0};

        /// @brief 이번 프레임의 dt
        double          dt_seconds   {0.0};

        /// @brief 누적 시간 (초 단위)
        double          time_seconds {0.0};

        /// @brief 이번 프레임의 입력 상태
        const framedot::input::InputState* input_state{nullptr};

        /// @brief 이번 프레임 입력 이벤트 기록 (오버플로로 일부 드랍 가능)
        const framedot::input::InputQueue* input_events{nullptr};

        /// @brief 잡 시스템(병렬 실행용)
        framedot::core::JobSystem* jobs{nullptr};

        /// @brief RenderPrep 기록 대상(프레임당). 게임/시스템은 여기에 그릴 내용을 기록.
        framedot::gfx::RenderQueue* render_queue{nullptr};
    };  

} // namespace framedot::core
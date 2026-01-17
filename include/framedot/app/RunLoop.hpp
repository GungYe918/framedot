// include/framedot/app/RunLoop.hpp
/**
 * @file RunLoop.hpp
 * @brief 프레임 루프(입력/업데이트/RenderPrep/픽셀화/Present) 실행기
 *
 * RenderPrep는 무엇을 그릴지(RenderQueue 기록)만 하고,
 * 실제 픽셀화는 엔진 내부 SoftwareRenderer가 수행
 */
#pragma once
#include <cstdint>

#include <framedot/gfx/PixelCanvas.hpp>
#include <framedot/gfx/RenderQueue.hpp>
#include <framedot/rhi/Surface.hpp>
#include <framedot/input/InputSource.hpp>
#include <framedot/core/FrameContext.hpp>


namespace framedot::app {

    class Client {
    public:
        virtual ~Client() = default;

        /// @brief 입력 처리를 위한 훅(기본 no-op). update 전에 호출
        virtual void on_input(const framedot::core::FrameContext& /*ctx*/) {}

        /// @brief 게임/앱 업데이트
        /// @return false를 반환하면 루프 종료
        virtual bool update(const framedot::core::FrameContext& ctx) = 0;

        /// @brief RenderPrep: 그릴 것을 RenderQueue에 기록한다. (픽셀 write 금지)
        virtual void render_prep(const framedot::core::FrameContext& ctx,
                                 framedot::gfx::RenderQueue& rq) = 0;
    };

    struct RunLoopConfig {
        bool   fixed_timestep = false;
        double fixed_dt = 1.0 / 60.0;
        double max_dt = 0.1;             // dt spike clamp
        
        std::uint64_t max_frames = 0;     // 0이면 무한

        /// @brief 워커 스레드 수(0=자동). SMP 비활성/플랫폼 제약이면 내부에서 0으로 축소될 수 있음.
        std::uint32_t worker_threads = 0;
    };

    int run(Client& client,
            framedot::gfx::PixelCanvas& canvas,
            framedot::rhi::Surface& surface,
            const RunLoopConfig& cfg,
            framedot::input::InputSource* input = nullptr);

} // namespace framedot::app
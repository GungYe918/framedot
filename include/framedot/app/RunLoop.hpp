// include/framedot/app/RunLoop.hpp
#pragma once
#include <cstdint>

#include <framedot/gfx/PixelCanvas.hpp>
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

        /// @brief 렌더 (캔버스에 그리기만 수행). 출력은 Surface가 담당
        virtual void render(const framedot::core::FrameContext& ctx,
                            framedot::gfx::PixelCanvas& canvas) = 0;
    };

    struct RunLoopConfig {
        bool   fixed_timestep = false;
        double fixed_dt = 1.0 / 60.0;

        double max_dt = 0.1;             // dt spike clamp
        std::uint64_t max_frames = 0;     // 0이면 무한
    };

    int run(Client& client,
            framedot::gfx::PixelCanvas& canvas,
            framedot::rhi::Surface& surface,
            const RunLoopConfig& cfg,
            framedot::input::InputSource* input = nullptr);

} // namespace framedot::app
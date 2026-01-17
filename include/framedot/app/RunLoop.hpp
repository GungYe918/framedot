// include/framedot/app/RunLoop.hpp
#pragma once
#include <cstdint>
#include <framedot/gfx/PixelCanvas.hpp>
#include <framedot/rhi/Surface.hpp>
#include <framedot/input/InputSource.hpp>
#include <framedot/input/InputQueue.hpp>


namespace framedot::app {

    class Client {
    public:
        virtual ~Client() = default;

        // dt: seconds
        virtual bool update(double dt_seconds) = 0;
        
        // render는 PixelCanvas에 그리기만 수행 (플랫폼 출력은 Surface가 담당)
        virtual void render(framedot::gfx::PixelCanvas& canvas) = 0;

        /// @brief 이번 프레임에 수집된 입력 이벤트를 전달받음
        virtual void on_input(const framedot::input::InputQueue& /*q*/) {  }
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

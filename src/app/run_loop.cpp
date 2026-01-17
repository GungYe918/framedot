// src/app/run_loop.cpp
#include <framedot/app/RunLoop.hpp>

#include <framedot/input/InputQueue.hpp>
#include <framedot/input/InputState.hpp>
#include <framedot/input/InputCollector.hpp>

#include <chrono>
#include <thread>


namespace framedot::app {

    int run(
        Client& client,
        framedot::gfx::PixelCanvas& canvas,
        framedot::rhi::Surface& surface,
        const RunLoopConfig& cfg,
        framedot::input::InputSource* input)
    {
        using clock = std::chrono::steady_clock;

        auto prev = clock::now();
        std::uint64_t frames = 0;
        double time_s = 0.0;

        double accumulator = 0.0;

        framedot::input::InputQueue input_q;
        framedot::input::InputState input_state;

        while (true) {
            if (cfg.max_frames != 0 && frames >= cfg.max_frames) break;

            const auto now = clock::now();
            std::chrono::duration<double> dt = now - prev;
            prev = now;

            double dt_s = dt.count();
            if (dt_s > cfg.max_dt) dt_s = cfg.max_dt;

            time_s += dt_s;

            /// ----------------------------
            /// 입력 수집 단계
            /// ----------------------------
            input_q.clear();
            input_state.begin_frame();

            framedot::input::InputCollector collector(input_state, input_q);

            if (input) {
                /// @brief 플랫폼 입력 소스에서 이벤트를 폴링해 collector로 주입
                input->pump(collector);
            }

            /// @brief 이번 프레임 컨텍스트 구성(프레임 동안 불변으로 취급)
            framedot::core::FrameContext ctx{};
            ctx.frame_index = frames;
            ctx.dt_seconds = dt_s;
            ctx.time_seconds = time_s;
            ctx.input_state = &input_state;
            ctx.input_events = &input_q;

            /// @brief update 전에 입력 훅 호출
            client.on_input(ctx);

            bool keep_running = true;

            if (cfg.fixed_timestep) {
                accumulator += dt_s;
                while (accumulator >= cfg.fixed_dt) {
                    framedot::core::FrameContext step = ctx;
                    step.dt_seconds = cfg.fixed_dt;

                    keep_running = client.update(step);
                    if (!keep_running) break;

                    accumulator -= cfg.fixed_dt;
                }
                if (!keep_running) break;
            } else {
                keep_running = client.update(ctx);
                if (!keep_running) break;
            }

            client.render(ctx, canvas);
            surface.present(canvas);

            ++frames;

            /// @brief 과점유 방지. 나중에 target FPS/VSync 정책으로 교체 가능.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return 0;
    }

} // namespace framedot::app
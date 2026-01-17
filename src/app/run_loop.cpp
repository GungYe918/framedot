// src/app/run_loop.cpp
#include <framedot/app/RunLoop.hpp>
#include <framedot/gfx/PixelCanvas.hpp>
#include <framedot/rhi/Surface.hpp>

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

        double accumulator = 0.0;

        framedot::input::InputQueue input_q;

        while (true) {
            if (cfg.max_frames != 0 && frames >= cfg.max_frames) break;

            const auto now = clock::now();
            std::chrono::duration<double> dt = now - prev;
            prev = now;

            double dt_s = dt.count();
            if (dt_s > cfg.max_dt) dt_s = cfg.max_dt;

            /// --- 입력 단계: 가능한 입력을 전부 수집 ---
            input_q.clear();
            if (input) {
                /// @brief 플랫폼 입력 소스에서 이벤트를 폴링해 큐에 적재
                input->pump(input_q);
            }

            /// @brief 입력 이벤트는 업데이트 전에 클라이언트에게 먼저 전달
            client.on_input(input_q);

            bool keep_running = true;

            if (cfg.fixed_timestep) {
                accumulator += dt_s;
                while (accumulator >= cfg.fixed_dt) {
                    keep_running = client.update(cfg.fixed_dt);
                    if (!keep_running) break;
                    accumulator -= cfg.fixed_dt;
                }
                if (!keep_running) break;
            } else {
                keep_running = client.update(dt_s);
                if (!keep_running) break;
            }

            client.render(canvas);
            surface.present(canvas);

            ++frames;

            /// @brief 과점유 방지. 나중에 VSync/target FPS 정책으로 교체.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        }

        return 0;
    }

} // namespace framedot::app
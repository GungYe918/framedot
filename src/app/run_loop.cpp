// src/app/run_loop.cpp
/**
 * @file run_loop.cpp
 * @brief RunLoop 구현부.
 *        입력 -> 업데이트 -> RenderPrep -> 소프트웨어 픽셀화 -> present 순서를 실행한다.
 *
 * 설계 원칙:
 * - fixed_timestep=true는 "실시간"이 아니라 "결정론적 스텝퍼(테스트/헤드리스)"로 동작한다.
 *   => 매 tick마다 dt=fixed_dt로 update 1회 수행, max_frames는 tick 수 기준.
 * - fixed_timestep=false는 실시간 dt 기반 루프.
 */
#include <framedot/app/RunLoop.hpp>

#include <framedot_internal/core/DefaultJobSystem.hpp>
#include <framedot/gfx/SoftwareRenderer.hpp>
#include <framedot/input/InputQueue.hpp>
#include <framedot/input/InputState.hpp>
#include <framedot/input/InputCollector.hpp>

#include <chrono>

namespace framedot::app {

    /// @brief 엔진 RunLoop 실행
    int run(
        Client& client,
        framedot::gfx::PixelCanvas& canvas,
        framedot::rhi::Surface& surface,
        const RunLoopConfig& cfg,
        framedot::input::InputSource* input)
    {
        using clock = std::chrono::steady_clock;

        framedot::input::InputState input_state;
        framedot::input::InputQueue input_queue;
        framedot::input::InputCollector collector(input_state, input_queue);

        framedot::core::JobSystem* jobs = framedot::core::create_default_jobsystem(cfg.worker_threads);

        framedot::gfx::RenderQueue rq;
        framedot::gfx::SoftwareRenderer sw;

        framedot::core::FrameContext ctx{};
        ctx.jobs = jobs;

        std::uint64_t tick = 0;
        double time_sec = 0.0;

        auto should_stop = [&](std::uint64_t tick_now) -> bool {
            return (cfg.max_frames != 0) && (tick_now >= cfg.max_frames);
        };

        // ----------------------------
        // fixed timestep = deterministic stepper (test/headless)
        // ----------------------------
        if (cfg.fixed_timestep) {
            while (true) {
                if (should_stop(tick)) break;

                // ----------------------------
                // [Stage 0] frame context
                // ----------------------------
                ctx.frame_index = tick;
                ctx.dt_seconds  = cfg.fixed_dt;
                ctx.time_seconds = time_sec;

                // ----------------------------
                // [Stage 1] input
                // ----------------------------
                input_state.begin_frame();
                input_queue.clear();

                if (input) {
                    input->pump(collector);
                }

                ctx.input_state  = &input_state;
                ctx.input_events = &input_queue;

                client.on_input(ctx);

                // ----------------------------
                // [Stage 2] update
                // ----------------------------
                const bool keep_running = client.update(ctx);
                jobs->wait_idle();
                if (!keep_running) break;

                // ----------------------------
                // [Stage 3] RenderPrep
                // ----------------------------
                rq.begin_frame();
                ctx.render_queue = &rq;

                client.render_prep(ctx, rq);
                jobs->wait_idle();

                // ----------------------------
                // [Stage 4] Raster
                // ----------------------------
                sw.execute(rq, canvas);

                // ----------------------------
                // [Stage 5] Present
                // ----------------------------
                surface.present(canvas.frame());

                // tick advance
                ++tick;
                time_sec += cfg.fixed_dt;
            }

            framedot::core::destroy_default_jobsystem(jobs);
            return 0;
        }

        // ----------------------------
        // variable timestep = realtime loop
        // ----------------------------
        auto prev = clock::now();

        while (true) {
            if (should_stop(tick)) break;

            const auto now = clock::now();
            std::chrono::duration<double> dt = now - prev;
            prev = now;

            double dt_s = dt.count();
            if (dt_s > cfg.max_dt) dt_s = cfg.max_dt;

            ctx.frame_index = tick;
            ctx.dt_seconds  = dt_s;
            time_sec += dt_s;
            ctx.time_seconds = time_sec;

            // input
            input_state.begin_frame();
            input_queue.clear();
            if (input) input->pump(collector);

            ctx.input_state  = &input_state;
            ctx.input_events = &input_queue;

            client.on_input(ctx);

            // update
            const bool keep_running = client.update(ctx);
            jobs->wait_idle();
            if (!keep_running) break;

            // render prep
            rq.begin_frame();
            ctx.render_queue = &rq;

            client.render_prep(ctx, rq);
            jobs->wait_idle();

            // raster + present
            sw.execute(rq, canvas);
            surface.present(canvas.frame());

            ++tick;
        }

        framedot::core::destroy_default_jobsystem(jobs);
        return 0;
    }

} // namespace framedot::app

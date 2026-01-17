// src/app/run_loop.cpp
#include <framedot/app/RunLoop.hpp>


#include <framedot/core/JobSystem.hpp>
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

        /// @brief 입력 상태는 "프레임마다 begin_frame -> 이벤트 적용" 순서로 갱신한다.
        framedot::input::InputState input_state;
        framedot::input::InputQueue input_queue;

        /// @brief InputCollector는 (상태 갱신 + 큐 기록)을 한 번에 수행한다.
        framedot::input::InputCollector collector(input_state, input_queue);

        /// @brief 잡 시스템 생성 (플랫폼에 따라 워커 0일 수도 있음)
        framedot::core::JobSystem* jobs = framedot::core::create_default_jobsystem(cfg.worker_threads);

        framedot::core::FrameContext ctx{};
        ctx.jobs = jobs;

        auto prev = clock::now();
        double acc = 0.0;

        std::uint64_t frames = 0;
        double time_sec = 0.0;

        while (true) {
            if (cfg.max_frames != 0 && frames >= cfg.max_frames) break;

            const auto now = clock::now();
            std::chrono::duration<double> dt = now - prev;
            prev = now;

            double dt_s = dt.count();
            if (dt_s > cfg.max_dt) dt_s = cfg.max_dt;

            /// ----------------------------
            /// [Stage 0] 프레임 컨텍스트 준비
            /// ----------------------------
            ctx.frame_index = frames;
            ctx.dt_seconds  = dt_s;
            time_sec += dt_s;
            ctx.time_seconds = time_sec;

            /// ----------------------------
            /// [Stage 1] 입력 수집 (플랫폼 스레드에서만!)
            /// ----------------------------
            input_state.begin_frame();
            input_queue.clear();

            if (input) {
                /// @brief 플랫폼 입력 소스에서 이벤트 폴링 -> collector에 push
                /// - 오버플로 정책으로 기록은 드랍될 수 있으나, 상태는 항상 최신 유지
                input->pump(collector);
            }

            ctx.input_state = &input_state;

            /// ----------------------------
            /// [Stage 2] 업데이트 (SMP 대상)
            /// ----------------------------
            bool keep_running = true;

            if (cfg.fixed_timestep) {
                acc += dt_s;
                while (acc >= cfg.fixed_dt) {
                    /// @brief 여기서부터는 "게임 스레드 로직"으로 간주
                    /// - 내부에서 jobs->enqueue()로 병렬 일을 던질 수 있음
                    ctx.dt_seconds = cfg.fixed_dt;
                    keep_running = client.update(ctx);
                    if (!keep_running) break;

                    /// @brief 프레임 내 병렬 작업이 있다면, 스테이지 경계에서 배리어를 거는 게 기본 패턴
                    jobs->wait_idle();

                    acc -= cfg.fixed_dt;
                }
                if (!keep_running) break;
            } else {
                keep_running = client.update(ctx);
                if (!keep_running) break;
                jobs->wait_idle();
            }

            /// ----------------------------
            /// [Stage 3] 렌더 (일단 단일 스레드)
            /// ----------------------------
            /// @brief PixelCanvas는 단일 버퍼라서 무턱대고 병렬 write하면 레이스 난다.
            /// - 추후 타일링/커맨드버퍼/더블버퍼로 병렬 렌더 확장 가능
            client.render(ctx, canvas);

            /// ----------------------------
            /// [Stage 4] Present (플랫폼 스레드 고정)
            /// ----------------------------
            surface.present(canvas);

            ++frames;

            /// @brief 과점유 방지(나중에 target fps 정책으로 교체)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        framedot::core::destroy_default_jobsystem(jobs);
        return 0;
    }

} // namespace framedot::app
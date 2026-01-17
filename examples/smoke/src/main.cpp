// examples/smoke/src/main.cpp
#include <framedot/framedot.hpp>
#include <framedot/ecs/World.hpp>
#include <framedot/core/Tasks.hpp>

#include <iostream>
#include <cmath>

using namespace framedot;

namespace {
class NullSurface final : public rhi::Surface {
public:
    void present(const gfx::PixelCanvas&) override {}
};

class SmokeClient final : public app::Client {
public:
    SmokeClient() {
        m_world.add_read_system(ecs::Phase::Update,
            [](const core::FrameContext& ctx, const ecs::World::Registry&) {
                volatile double acc = 0.0;
                for (int i = 0; i < 200000; ++i) acc += std::sin(i * 0.001);
                (void)ctx;
            });

        m_world.add_read_system(ecs::Phase::Update,
            [](const core::FrameContext& ctx, const ecs::World::Registry&) {
                volatile double acc = 1.0;
                for (int i = 1; i < 200000; ++i) acc *= 1.0000001;
                (void)ctx;
            });

        m_world.add_write_system(ecs::Phase::Update,
            [](const core::FrameContext& ctx, ecs::World::Registry&) {
                (void)ctx;
            });
    }

    bool update(const core::FrameContext& ctx) override {
        if (ctx.input_state && ctx.input_state->key_just_pressed(input::Key::Escape)) {
            return false;
        }

        // 1) ECS tick: ReadOnly는 엔진이 Engine lane으로 병렬 실행 (World 내부 로직)
        m_world.tick(ctx);

        // 2) 유저가 명시적으로 TaskGroup(User lane) 사용해보기
        if (ctx.jobs && ctx.jobs->worker_count() > 0) {
            core::TaskGroup tg(ctx.jobs, core::JobLane::User);

            core::TaskValue<double> v0;
            core::TaskValue<double> v1;

            core::run_value(tg, v0, [&]() -> double {
                volatile double acc = 0.0;
                for (int i = 0; i < 250000; ++i) acc += std::sin(i * 0.002);
                return static_cast<double>(acc);
            });

            core::run_value(tg, v1, [&]() -> double {
                volatile double acc = 1.0;
                for (int i = 1; i < 250000; ++i) acc *= 1.00000001;
                return static_cast<double>(acc);
            });

            // tg는 scope 종료 시 자동 wait하지만, smoke에서는 명시적으로 보여주자
            tg.wait();

            if ((ctx.frame_index % 60ull) == 0ull) {
                std::cout << "[smoke] workers=" << ctx.jobs->worker_count()
                          << " user_tasks=(" << v0.get() << ", " << v1.get() << ")\n";
            }
        } else {
            if ((ctx.frame_index % 60ull) == 0ull) {
                std::cout << "[smoke] no workers (single-thread path)\n";
            }
        }

        return true;
    }

    void render(const core::FrameContext& ctx, gfx::PixelCanvas& canvas) override {
        (void)ctx;
        canvas.clear(gfx::Color::rgba(0,0,0,255));
        canvas.put_pixel(1, 1, gfx::Color::rgba(255,255,255,255));
    }

private:
    ecs::World m_world;
};
}

int main() {
    gfx::PixelCanvas canvas(64, 32);
    NullSurface surface;
    SmokeClient client;

    app::RunLoopConfig cfg;
    cfg.fixed_timestep = true;
    cfg.fixed_dt = 1.0/60.0;
    cfg.max_frames = 256;

    std::cout << "framedot smoke running...\n";
    return app::run(client, canvas, surface, cfg);
}
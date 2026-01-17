// examples/smoke/src/main.cpp
#include <framedot/framedot.hpp>
#include <framedot/ecs/World.hpp>

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
        // 엔티티 하나 만들고 Position 같은 거 있다고 가정해도 되지만,
        // 지금은 "읽기 전용 시스템 병렬"만 검증하자.

        m_world.add_read_system(ecs::Phase::Update,
            [](const core::FrameContext& ctx, const ecs::World::Registry&) {
                // 무거운 계산(예: 2ms 정도 태우기)
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

        // Write 시스템은 직렬로 실행되는지 확인용(여기선 아무것도 안함)
        m_world.add_write_system(ecs::Phase::Update,
            [](const core::FrameContext& ctx, ecs::World::Registry&) {
                (void)ctx;
            });
    }

    bool update(const core::FrameContext& ctx) override {
        if (ctx.input_state && ctx.input_state->key_just_pressed(input::Key::Escape)) {
            return false;
        }

        // ECS tick: ReadOnly는 병렬, Write는 직렬
        m_world.tick(ctx);
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

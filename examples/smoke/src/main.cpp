#include <iostream>
#include <framedot/framedot.hpp>

using namespace framedot;

namespace {
class NullSurface final : public rhi::Surface {
public:
    void present(const gfx::PixelCanvas&) override {
        // do nothing
    }
};

class SmokeClient final : public app::Client {
public:
    bool update(double /*dt*/) override {
        ++ticks;
        return ticks < 10; // 10프레임 돌고 종료
    }

    void render(gfx::PixelCanvas& canvas) override {
        canvas.clear(gfx::Color::rgba(0,0,0,255));
        // 더미 픽셀 하나 찍기
        canvas.put_pixel(1, 1, gfx::Color::rgba(255,255,255,255));
    }

private:
    int ticks = 0;
};
}

int main() {
    gfx::PixelCanvas canvas(64, 32);
    NullSurface surface;
    SmokeClient client;

    app::RunLoopConfig cfg;
    cfg.fixed_timestep = true;
    cfg.fixed_dt = 1.0/60.0;
    cfg.max_frames = 0;

    std::cout << "framedot smoke running...\n";
    return app::run(client, canvas, surface, cfg);
}

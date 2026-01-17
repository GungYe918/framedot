// platform/terminal_ncurses/examples/term_demo.cpp
#include <framedot/gfx/PixelCanvas.hpp>
#include <framedot/gfx/RenderQueue.hpp>
#include <framedot/gfx/SoftwareRenderer.hpp>
#include <framedot/gfx/Color.hpp>

#include <framedot_platform/TerminalSurface.hpp>

#include <chrono>
#include <thread>
#include <cstdint>

using namespace framedot;

static gfx::ColorRGBA8 rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) {
    return gfx::ColorRGBA8{r, g, b, a};
}

int main() {
    gfx::PixelCanvas canvas(120, 40);
    platform::terminal::TerminalSurface surface;

    gfx::RenderQueue rq;
    gfx::SoftwareRenderer sw;

    int t = 0;

    while (true) {
        const int key = surface.poll_key();
        if (key == 'q' || key == 'Q') break;

        rq.begin_frame();

        // 배경
        rq.clear(rgba(0, 0, 0));

        // 지면
        rq.hline(0, 119, 30, rgba(0, 255, 0));

        // 움직이는 박스
        const int x0 = (t / 2) % 100;
        const int y0 = 10 + (t % 10);
        rq.fill_rect(x0, y0, 20, 8, rgba(255, 0, 0));

        // 컬러 스트라이프
        rq.hline(0, 119, 2, rgba(255, 255, 0));
        rq.hline(0, 119, 3, rgba(0, 255, 255));
        rq.hline(0, 119, 4, rgba(255, 0, 255));

        sw.execute(rq, canvas);
        surface.present(canvas.frame());

        ++t;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
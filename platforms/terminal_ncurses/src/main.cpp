#include <framedot/gfx/PixelCanvas.hpp>
#include <framedot/gfx/Color.hpp>
#include <framedot_platform/TerminalSurface.hpp>

#include <chrono>
#include <thread>
#include <cstdint>

using namespace framedot;

static gfx::ColorRGBA8 rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a=255) {
    return gfx::ColorRGBA8{r,g,b,a};
}

int main() {
    // 캔버스는 터미널 크기와 맞춰도 되지만, 일단 고정으로 MVP
    // (TerminalSurface가 알아서 잘리는 부분만 출력함)
    gfx::PixelCanvas canvas(120, 40);
    platform::terminal::TerminalSurface surface;

    int t = 0;
    while (true) {
        const int key = surface.poll_key();
        if (key == 'q' || key == 'Q') break;

        // 배경
        canvas.clear(rgba(0, 0, 0));

        // 움직이는 컬러 바 + 박스
        const int x0 = (t / 2) % 110;
        const int y0 = 10 + (t % 10);

        // 간단한 지형선
        for (int x = 0; x < 120; ++x) {
            canvas.put_pixel(x, 30, rgba(0, 255, 0));
        }

        // 박스
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 20; ++x) {
                canvas.put_pixel(x0 + x, y0 + y, rgba(255, 0, 0));
            }
        }

        // 색상 스트라이프
        for (int x = 0; x < 120; ++x) {
            canvas.put_pixel(x, 2, rgba(255, 255, 0));
            canvas.put_pixel(x, 3, rgba(0, 255, 255));
            canvas.put_pixel(x, 4, rgba(255, 0, 255));
        }

        surface.present(canvas);

        ++t;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
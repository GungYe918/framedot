// src/gfx/software_renderer.cpp
/**
 * @file software_renderer.cpp
 * @brief SoftwareRenderer 구현부(클리핑 + 빠른 fill).
 */
#include <framedot/gfx/SoftwareRenderer.hpp>
#include <algorithm>


namespace framedot::gfx {

    static inline int clampi(int v, int lo, int hi) noexcept {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    /// @brief (x0,y0,w,h) 사각형을 캔버스 경계로 클리핑한다. 결과가 비면 false
    static bool clip_rect(int& x, int& y, int& w, int& h, int W, int H) noexcept {
        if (w <= 0 || h <= 0) return false;

        int x1 = x + w;
        int y1 = y + h;

        x = clampi(x, 0, W);
        y = clampi(y, 0, H);
        x1 = clampi(x1, 0, W);
        y1 = clampi(y1, 0, H);

        w = x1 - x;
        h = y1 - y;
        return (w > 0 && h > 0);
    }

    /// @brief y가 유효한지 체크하고, [x0,x1]를 [0,W-1]로 클리핑
    static bool clip_hline(int& x0, int& x1, int y, int W, int H) noexcept {
        if (y < 0 || y >= H) return false;
        if (x0 > x1) std::swap(x0, x1);
        x0 = clampi(x0, 0, W - 1);
        x1 = clampi(x1, 0, W - 1);
        return x0 <= x1;
    }

    /// @brief x가 유효한지 체크하고, [y0,y1]를 [0,H-1]로 클리핑
    static bool clip_vline(int x, int& y0, int& y1, int W, int H) noexcept {
        (void)W;
        if (x < 0 || x >= W) return false;
        if (y0 > y1) std::swap(y0, y1);
        y0 = clampi(y0, 0, H - 1);
        y1 = clampi(y1, 0, H - 1);
        return y0 <= y1;
    }

    void SoftwareRenderer::execute(const RenderQueue& rq, PixelCanvas& out) noexcept {
        const int W = static_cast<int>(out.width());
        const int H = static_cast<int>(out.height());
        if (W <= 0 || H <= 0) return;

        auto buf = out.pixels();
        if (buf.empty()) return;

        for (std::size_t i = 0; i < rq.size(); ++i) {
            const auto& c = rq.data()[i];
            const std::uint32_t packed = PixelCanvas::pack(c.color);

            switch(c.op) {
                case RenderQueue::Op::Clear: {
                    std::fill(buf.begin(), buf.end(), packed);
                    break;
                }

                case RenderQueue::Op::PutPixel: {
                    const int x = c.x0;
                    const int y = c.y0;
                    if (x < 0 || y < 0 || x >= W || y >= H) break;
                    buf[static_cast<std::size_t>(y) * static_cast<std::size_t>(W) + static_cast<std::size_t>(x)] = packed;
                    break;
                }

                case RenderQueue::Op::FillRect: {
                    int x = c.x0;
                    int y = c.y0;
                    int w = c.x1;
                    int h = c.y1;

                    if (!clip_rect(x, y, w, h, W, H)) break;

                    for (int yy = 0; yy < h; ++yy) {
                        const std::size_t row = static_cast<std::size_t>(y + yy) * static_cast<std::size_t>(W);
                        std::uint32_t* dst = buf.data() + row + static_cast<std::size_t>(x);
                        std::fill(dst, dst + w, packed);
                    }
                    break;
                }

                case RenderQueue::Op::HLine: {
                    int x0 = c.x0;
                    int x1 = c.x1;
                    const int y = c.y0;

                    if (!clip_hline(x0, x1, y, W, H)) break;

                    const std::size_t row = static_cast<std::size_t>(y) * static_cast<std::size_t>(W);
                    std::uint32_t* dst = buf.data() + row + static_cast<std::size_t>(x0);
                    std::fill(dst, dst + (x1 - x0 + 1), packed);
                    break;
                }

                case RenderQueue::Op::VLine: {
                    const int x = c.x0;
                    int y0 = c.y0;
                    int y1 = c.y1;

                    if (!clip_vline(x, y0, y1, W, H)) break;

                    for (int y = y0; y <= y1; ++y) {
                        buf[static_cast<std::size_t>(y) * static_cast<std::size_t>(W) + static_cast<std::size_t>(x)] = packed;
                    }
                    break;
                }
            }
        }

    }


} // namespace framedot::gfx
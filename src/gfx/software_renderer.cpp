// src/gfx/software_renderer.cpp
/**
 * @file software_renderer.cpp
 * @brief SoftwareRenderer 구현부
 * @brief sort_key를 반영해 커맨드 실행 순서 정렬
 * @brief 동적 할당 없이 std::array<idx>로 인덱스 정렬만 수행
 */
#include <framedot/gfx/SoftwareRenderer.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

namespace framedot::gfx {

    /// @brief RGBA8888 (0xRRGGBBAA) 픽셀에서 채널 추출
    constexpr std::uint8_t pr(std::uint32_t p) noexcept { return (p >> 24) & 0xFF; }
    constexpr std::uint8_t pg(std::uint32_t p) noexcept { return (p >> 16) & 0xFF; }
    constexpr std::uint8_t pb(std::uint32_t p) noexcept { return (p >>  8) & 0xFF; }
    constexpr std::uint8_t pa(std::uint32_t p) noexcept { return (p >>  0) & 0xFF; }

    constexpr std::uint32_t pack_rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) noexcept {
        return (std::uint32_t(r) << 24) | (std::uint32_t(g) << 16) | (std::uint32_t(b) << 8) | std::uint32_t(a);
    }

    /// @brief src-over 알파 블렌딩 (dst 위에 src를 올림)
    inline std::uint32_t blend_over(std::uint32_t dst, ColorRGBA8 src) noexcept {
        const std::uint32_t sa = src.a;
        if (sa == 255) return pack_rgba(src.r, src.g, src.b, src.a);
        if (sa == 0)   return dst;

        const std::uint32_t da = pa(dst);

        const std::uint32_t sr = src.r;
        const std::uint32_t sg = src.g;
        const std::uint32_t sb = src.b;

        const std::uint32_t dr = pr(dst);
        const std::uint32_t dg = pg(dst);
        const std::uint32_t db = pb(dst);

        // out = src*sa + dst*(1-sa)
        const std::uint32_t inv = 255 - sa;

        const std::uint32_t or_ = (sr * sa + dr * inv) / 255;
        const std::uint32_t og_ = (sg * sa + dg * inv) / 255;
        const std::uint32_t ob_ = (sb * sa + db * inv) / 255;

        // 알파도 단순 합성
        const std::uint32_t oa_ = (sa + (da * inv) / 255);
        return pack_rgba((std::uint8_t)or_, (std::uint8_t)og_, (std::uint8_t)ob_, (std::uint8_t)oa_);
    }

    inline bool in_bounds(int x, int y, int w, int h) noexcept {
        return (x >= 0 && y >= 0 && x < w && y < h);
    }

    /// @brief Bresenham line
    template <class Plot>
    void raster_line(int x0, int y0, int x1, int y1, Plot&& plot) noexcept {
        int dx = std::abs(x1 - x0);
        int sx = (x0 < x1) ? 1 : -1;
        int dy = -std::abs(y1 - y0);
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx + dy;

        while (true) {
            plot(x0, y0);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    void SoftwareRenderer::execute(const RenderQueue& rq, PixelCanvas& out) noexcept {
        const std::size_t n = rq.size();
        if (n == 0) return;

        const int W = (int)out.width();
        const int H = (int)out.height();
        auto pix = out.pixels();

        auto write_px = [&](int x, int y, ColorRGBA8 c) noexcept {
            if (!in_bounds(x, y, W, H)) return;
            const std::size_t idx = (std::size_t)y * (std::size_t)W + (std::size_t)x;
            if (c.a == 255) {
                pix[idx] = PixelCanvas::pack(c);
            } else if (c.a != 0) {
                pix[idx] = blend_over(pix[idx], c);
            }
        };

        // 1) 인덱스 배열을 sort_key 기준으로 정렬 (동적할당 없이)
        std::array<std::uint16_t, RenderQueue::kMax> order{};
        for (std::size_t i = 0; i < n; ++i) order[i] = (std::uint16_t)i;

        const RenderQueue::Cmd* cmds = rq.data();
        std::sort(order.begin(), order.begin() + (std::ptrdiff_t)n,
                  [&](std::uint16_t a, std::uint16_t b) {
                      return cmds[a].sort_key < cmds[b].sort_key;
                  });

        // 2) 커맨드 실행
        for (std::size_t oi = 0; oi < n; ++oi) {
            const RenderQueue::Cmd& c = cmds[order[oi]];

            switch (c.op) {
            case RenderQueue::Op::Clear: {
                const std::uint32_t p = PixelCanvas::pack(c.color);
                for (std::size_t i = 0; i < pix.size(); ++i) pix[i] = p;
                break;
            }
            case RenderQueue::Op::PutPixel: {
                write_px(c.x0, c.y0, c.color);
                break;
            }
            case RenderQueue::Op::FillRect:
            case RenderQueue::Op::BlendRect: { // BlendRect는 이제 “명시적으로 알파를 의도”하는 의미만 남음
                const int x0 = c.x0, y0 = c.y0;
                const int x1 = c.x0 + c.x1;
                const int y1 = c.y0 + c.y1;

                for (int y = y0; y < y1; ++y) {
                    if (y < 0 || y >= H) continue;
                    for (int x = x0; x < x1; ++x) {
                        if (x < 0 || x >= W) continue;
                        write_px(x, y, c.color);
                    }
                }
                break;
            }
            case RenderQueue::Op::HLine: {
                const int y = c.y0;
                if (y < 0 || y >= H) break;
                int x0 = c.x0, x1 = c.x1;
                if (x0 > x1) std::swap(x0, x1);
                for (int x = x0; x <= x1; ++x) write_px(x, y, c.color);
                break;
            }
            case RenderQueue::Op::VLine: {
                const int x = c.x0;
                if (x < 0 || x >= W) break;
                int y0 = c.y0, y1 = c.y1;
                if (y0 > y1) std::swap(y0, y1);
                for (int y = y0; y <= y1; ++y) write_px(x, y, c.color);
                break;
            }
            case RenderQueue::Op::Line: {
                raster_line(c.x0, c.y0, c.x1, c.y1, [&](int x, int y) {
                    write_px(x, y, c.color);
                });
                break;
            }
            case RenderQueue::Op::RectOutline: {
                const int t = (int)c.u0;
                if (t <= 0) break;
                const int x = c.x0, y = c.y0;
                const int w = c.x1, h = c.y1;

                for (int i = 0; i < t; ++i) {
                    // top/bot
                    for (int xx = x; xx < x + w; ++xx) {
                        write_px(xx, y + i, c.color);
                        write_px(xx, y + h - 1 - i, c.color);
                    }
                    // left/right
                    for (int yy = y; yy < y + h; ++yy) {
                        write_px(x + i, yy, c.color);
                        write_px(x + w - 1 - i, yy, c.color);
                    }
                }
                break;
            }
            case RenderQueue::Op::FillCircle:
            case RenderQueue::Op::Circle: {
                const int cx = c.x0, cy = c.y0, r = c.x1;
                if (r <= 0) break;

                int x = r;
                int y = 0;
                int err = 0;

                auto plot8 = [&](int px, int py) {
                    const int pts[8][2] = {
                        { cx + px, cy + py }, { cx + py, cy + px },
                        { cx - py, cy + px }, { cx - px, cy + py },
                        { cx - px, cy - py }, { cx - py, cy - px },
                        { cx + py, cy - px }, { cx + px, cy - py },
                    };
                    for (auto& pt : pts) write_px(pt[0], pt[1], c.color);
                };

                auto hspan = [&](int yline, int x0, int x1) {
                    if (yline < 0 || yline >= H) return;
                    if (x0 > x1) std::swap(x0, x1);
                    for (int xx = x0; xx <= x1; ++xx) write_px(xx, yline, c.color);
                };

                while (x >= y) {
                    if (c.op == RenderQueue::Op::Circle) {
                        plot8(x, y);
                    } else {
                        hspan(cy + y, cx - x, cx + x);
                        hspan(cy - y, cx - x, cx + x);
                        hspan(cy + x, cx - y, cx + y);
                        hspan(cy - x, cx - y, cx + y);
                    }

                    if (err <= 0) {
                        y += 1;
                        err += 2*y + 1;
                    }
                    if (err > 0) {
                        x -= 1;
                        err -= 2*x + 1;
                    }
                }
                break;
            }
            default:
                break;
            }
        }
    }
    
} // namespace framedot::gfx
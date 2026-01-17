// src/gfx/software_renderer.cpp
/**
 * @file software_renderer.cpp
 * @brief SoftwareRenderer 구현부
 * @brief sort_key를 반영해 커맨드 실행 순서 정렬
 * @brief 타일 기반 병렬 래스터 (write 병렬화 시작)
 */
#include <framedot/gfx/SoftwareRenderer.hpp>
#include <framedot/core/Tasks.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>


namespace framedot::gfx {

    constexpr std::uint8_t pr(std::uint32_t p) noexcept { return (p >> 24) & 0xFF; }
    constexpr std::uint8_t pg(std::uint32_t p) noexcept { return (p >> 16) & 0xFF; }
    constexpr std::uint8_t pb(std::uint32_t p) noexcept { return (p >>  8) & 0xFF; }
    constexpr std::uint8_t pa(std::uint32_t p) noexcept { return (p >>  0) & 0xFF; }

    constexpr std::uint32_t pack_rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) noexcept {
        return (std::uint32_t(r) << 24) | (std::uint32_t(g) << 16) | (std::uint32_t(b) << 8) | std::uint32_t(a);
    }

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

        const std::uint32_t inv = 255 - sa;

        const std::uint32_t or_ = (sr * sa + dr * inv) / 255;
        const std::uint32_t og_ = (sg * sa + dg * inv) / 255;
        const std::uint32_t ob_ = (sb * sa + db * inv) / 255;

        const std::uint32_t oa_ = (sa + (da * inv) / 255);
        return pack_rgba((std::uint8_t)or_, (std::uint8_t)og_, (std::uint8_t)ob_, (std::uint8_t)oa_);
    }

    inline bool in_bounds(int x, int y, int w, int h) noexcept {
        return (x >= 0 && y >= 0 && x < w && y < h);
    }

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

    static inline ColorRGBA8 modulate(ColorRGBA8 src, ColorRGBA8 tint) noexcept {
        auto mul = [](std::uint8_t a, std::uint8_t b) -> std::uint8_t {
            return (std::uint16_t(a) * std::uint16_t(b)) / 255;
        };
        return ColorRGBA8{
            mul(src.r, tint.r),
            mul(src.g, tint.g),
            mul(src.b, tint.b),
            mul(src.a, tint.a),
        };
    }

    // ---- 실행(타일 단위) ----
    struct Tile {
        int x0, y0, x1, y1; // [x0,x1), [y0,y1)
    };

    static void execute_tile_(const RenderQueue& rq,
                              const RenderQueue::Cmd* cmds,
                              const std::uint16_t* order,
                              std::size_t n,
                              PixelCanvas& out,
                              const Tile& t) noexcept {
        const int W = (int)out.width();
        const int H = (int)out.height();
        auto pix = out.pixels();

        auto write_px = [&](int x, int y, ColorRGBA8 c) noexcept {
            if (x < t.x0 || x >= t.x1 || y < t.y0 || y >= t.y1) return;
            if (!in_bounds(x, y, W, H)) return;
            const std::size_t idx = (std::size_t)y * (std::size_t)W + (std::size_t)x;
            if (c.a == 255) {
                pix[idx] = PixelCanvas::pack(c);
            } else if (c.a != 0) {
                pix[idx] = blend_over(pix[idx], c);
            }
        };

        auto fill_tile = [&](std::uint32_t p) noexcept {
            for (int y = t.y0; y < t.y1; ++y) {
                if (y < 0 || y >= H) continue;
                const std::size_t row = (std::size_t)y * (std::size_t)W;
                for (int x = t.x0; x < t.x1; ++x) {
                    if (x < 0 || x >= W) continue;
                    pix[row + (std::size_t)x] = p;
                }
            }
        };

        for (std::size_t oi = 0; oi < n; ++oi) {
            const RenderQueue::Cmd& c = cmds[order[oi]];

            switch (c.op) {
            case RenderQueue::Op::Clear: {
                fill_tile(PixelCanvas::pack(c.color));
                break;
            }
            case RenderQueue::Op::PutPixel: {
                write_px(c.x0, c.y0, c.color);
                break;
            }
            case RenderQueue::Op::FillRect:
            case RenderQueue::Op::BlendRect: {
                const int rx0 = c.x0;
                const int ry0 = c.y0;
                const int rx1 = c.x0 + c.x1;
                const int ry1 = c.y0 + c.y1;

                const int sx0 = (rx0 > t.x0) ? rx0 : t.x0;
                const int sy0 = (ry0 > t.y0) ? ry0 : t.y0;
                const int sx1 = (rx1 < t.x1) ? rx1 : t.x1;
                const int sy1 = (ry1 < t.y1) ? ry1 : t.y1;

                for (int y = sy0; y < sy1; ++y) {
                    if (y < 0 || y >= H) continue;
                    for (int x = sx0; x < sx1; ++x) {
                        if (x < 0 || x >= W) continue;
                        write_px(x, y, c.color);
                    }
                }
                break;
            }
            case RenderQueue::Op::RectOutline: {
                const int tpx = (int)c.u0;
                if (tpx <= 0) break;
                const int x = c.x0, y = c.y0;
                const int w = c.x1, h = c.y1;

                for (int i = 0; i < tpx; ++i) {
                    for (int xx = x; xx < x + w; ++xx) {
                        write_px(xx, y + i, c.color);
                        write_px(xx, y + h - 1 - i, c.color);
                    }
                    for (int yy = y; yy < y + h; ++yy) {
                        write_px(x + i, yy, c.color);
                        write_px(x + w - 1 - i, yy, c.color);
                    }
                }
                break;
            }
            case RenderQueue::Op::Line: {
                raster_line(c.x0, c.y0, c.x1, c.y1, [&](int x, int y) {
                    write_px(x, y, c.color);
                });
                break;
            }
            case RenderQueue::Op::HLine: {
                const int y = c.y0;
                int x0 = c.x0, x1 = c.x1;
                if (x0 > x1) std::swap(x0, x1);
                for (int x = x0; x <= x1; ++x) write_px(x, y, c.color);
                break;
            }
            case RenderQueue::Op::VLine: {
                const int x = c.x0;
                int y0 = c.y0, y1 = c.y1;
                if (y0 > y1) std::swap(y0, y1);
                for (int y = y0; y <= y1; ++y) write_px(x, y, c.color);
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

                    if (err <= 0) { y += 1; err += 2*y + 1; }
                    if (err > 0)  { x -= 1; err -= 2*x + 1; }
                }
                break;
            }
            case RenderQueue::Op::BlitSprite: {
                const auto* src = (const std::uint32_t*)rq.payload0(order[oi]);
                const int w = c.x1;
                const int h = c.y1;
                const int dx0 = c.x0;
                const int dy0 = c.y0;
                const int stride = (int)c.u0;

                if (!src || w <= 0 || h <= 0 || stride <= 0) break;

                // 타일과 교집합만
                const int sx0 = (dx0 > t.x0) ? dx0 : t.x0;
                const int sy0 = (dy0 > t.y0) ? dy0 : t.y0;
                const int sx1 = ((dx0 + w) < t.x1) ? (dx0 + w) : t.x1;
                const int sy1 = ((dy0 + h) < t.y1) ? (dy0 + h) : t.y1;

                for (int y = sy0; y < sy1; ++y) {
                    const int sy = y - dy0;
                    if (y < 0 || y >= H) continue;

                    for (int x = sx0; x < sx1; ++x) {
                        const int sx = x - dx0;
                        if (x < 0 || x >= W) continue;

                        const std::uint32_t sp = src[(std::size_t)sy * (std::size_t)stride + (std::size_t)sx];
                        ColorRGBA8 sc{
                            pr(sp), pg(sp), pb(sp), pa(sp)
                        };
                        sc = modulate(sc, c.color);
                        write_px(x, y, sc);
                    }
                }
                break;
            }
            case RenderQueue::Op::Text: {
                const std::uint32_t ofs = (std::uint32_t)c.x1;
                const std::uint32_t len = (std::uint32_t)c.y1;
                const char* s = rq.text_data(ofs);

                // 지금은 debug text: 각 문자를 "작은 블록"으로 표시(폰트는 다음 스텝)
                int penx = c.x0;
                int peny = c.y0;

                const int scale = (c.u0 == 0) ? 1 : (int)c.u0;

                for (std::uint32_t i = 0; i < len; ++i) {
                    const char ch = s[i];
                    if (ch == '\n') { penx = c.x0; peny += 8 * scale; continue; }

                    // 4x6 블록
                    const int bw = 4 * scale;
                    const int bh = 6 * scale;

                    for (int yy = 0; yy < bh; ++yy) {
                        for (int xx = 0; xx < bw; ++xx) {
                            // 공백은 스킵
                            if (ch == ' ') continue;
                            write_px(penx + xx, peny + yy, c.color);
                        }
                    }
                    penx += (bw + 1);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    void SoftwareRenderer::execute(const RenderQueue& rq, PixelCanvas& out) noexcept {
        framedot::core::FrameContext dummy{};
        execute(dummy, rq, out);
    }

    void SoftwareRenderer::execute(const framedot::core::FrameContext& ctx,
                                   const RenderQueue& rq,
                                   PixelCanvas& out) noexcept {
        const std::size_t n = rq.size();
        if (n == 0) return;

        // 1) order 정렬 (고정 배열)
        std::array<std::uint16_t, RenderQueue::kMax> order{};
        for (std::size_t i = 0; i < n; ++i) order[i] = (std::uint16_t)i;

        const RenderQueue::Cmd* cmds = rq.data();
        std::sort(order.begin(), order.begin() + (std::ptrdiff_t)n,
                  [&](std::uint16_t a, std::uint16_t b) {
                      return cmds[a].sort_key < cmds[b].sort_key;
                  });

        const int W = (int)out.width();
        const int H = (int)out.height();

        // 2) 타일 병렬
        constexpr int kTile = 32;

        const int tiles_x = (W + kTile - 1) / kTile;
        const int tiles_y = (H + kTile - 1) / kTile;
        const int tile_count = tiles_x * tiles_y;

        const bool can_parallel = (ctx.jobs && ctx.jobs->worker_count() > 0 && tile_count >= 2);

        if (!can_parallel) {
            Tile whole{0, 0, W, H};
            execute_tile_(rq, cmds, order.data(), n, out, whole);
            return;
        }

        framedot::core::TaskGroup tg(ctx.jobs, framedot::core::JobLane::Engine);

        for (int ty = 0; ty < tiles_y; ++ty) {
            for (int tx = 0; tx < tiles_x; ++tx) {
                const int x0 = tx * kTile;
                const int y0 = ty * kTile;
                const int x1 = (x0 + kTile < W) ? (x0 + kTile) : W;
                const int y1 = (y0 + kTile < H) ? (y0 + kTile) : H;

                Tile tile{x0, y0, x1, y1};

                tg.run([&, tile]() noexcept {
                    execute_tile_(rq, cmds, order.data(), n, out, tile);
                });
            }
        }

        tg.wait();
    }

} // namespace framedot::gfx
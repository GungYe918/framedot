// include/framedot/ecs/systems/RenderPrep2D.hpp
/**
 * @file RenderPrep2D.hpp
 * @brief Basic2D(+Sprite2D/Text2D) 컴포넌트를 RenderQueue 커맨드로 변환하는 RenderPrep 시스템.
 *
 * 개선 포인트:
 * - 병렬 구간에서 EnTT registry 접근을 하지 않고,
 *   메인에서 snapshot(POD 배열)만 만든다.
 * - 워커는 RenderQueue push만 수행한다.
 */
#pragma once
#include <framedot/core/Tasks.hpp>
#include <framedot/ecs/World.hpp>
#include <framedot/ecs/components/Basic2D.hpp>
#include <framedot/gfx/RenderQueue.hpp>

#include <array>
#include <cstdint>
#include <string_view>

namespace framedot::ecs::systems {

    static constexpr std::size_t kMaxItems = framedot::gfx::RenderQueue::kMax;

    struct RectItem {
        int x, y, w, h;
        framedot::gfx::ColorRGBA8 color;
        std::uint32_t sort_key;
        std::uint16_t outline_px;
        bool outline;
    };

    struct SpriteItem {
        int x, y;
        const std::uint32_t* pixels;
        int w, h;
        std::uint16_t stride;
        framedot::gfx::ColorRGBA8 tint;
        std::uint32_t sort_key;
    };

    struct TextItem {
        int x, y;
        const char* text;
        std::uint16_t len;
        framedot::gfx::ColorRGBA8 color;
        std::uint8_t scale;
        std::uint32_t sort_key;
    };

    inline void install_render_prep_2d(World& world) {
        world.add_read_system(Phase::RenderPrep,
            [](const framedot::core::FrameContext& ctx, const World::Registry& reg) {
                auto* rq = ctx.render_queue;
                if (!rq) return;

                // ----------------------------
                // 1) snapshot gather (single thread)
                // ----------------------------
                std::array<RectItem, kMaxItems> rects{};
                std::array<SpriteItem, kMaxItems> sprites{};
                std::array<TextItem, kMaxItems> texts{};

                std::size_t rc = 0, sc = 0, tc = 0;

                // Rect
                {
                    auto view = reg.view<const framedot::ecs::Transform2D,
                                        const framedot::ecs::Rect2D>();
                    for (auto e : view) {
                        if (rc >= rects.size()) break;

                        const auto& t = view.get<const framedot::ecs::Transform2D>(e);
                        const auto& r = view.get<const framedot::ecs::Rect2D>(e);

                        std::uint32_t sort_key = 0;
                        if (const auto* ro = reg.try_get<framedot::ecs::RenderOrder2D>(e)) {
                            sort_key = ro->sort_key;
                        }

                        RectItem it{};
                        it.x = (int)t.position.x;
                        it.y = (int)t.position.y;
                        it.w = (int)r.size.x;
                        it.h = (int)r.size.y;
                        it.color = r.color;
                        it.sort_key = sort_key;
                        it.outline_px = r.outline_px;
                        it.outline = (r.outline_px > 0);
                        rects[rc++] = it;
                    }
                }

                // Sprite
                {
                    auto view = reg.view<const framedot::ecs::Transform2D,
                                        const framedot::ecs::Sprite2D>();
                    for (auto e : view) {
                        if (sc >= sprites.size()) break;

                        const auto& t = view.get<const framedot::ecs::Transform2D>(e);
                        const auto& s = view.get<const framedot::ecs::Sprite2D>(e);

                        if (!s.pixels || s.width <= 0 || s.height <= 0) continue;

                        std::uint32_t sort_key = 0;
                        if (const auto* ro = reg.try_get<framedot::ecs::RenderOrder2D>(e)) {
                            sort_key = ro->sort_key;
                        }

                        SpriteItem it{};
                        it.x = (int)t.position.x;
                        it.y = (int)t.position.y;
                        it.pixels = s.pixels;
                        it.w = s.width;
                        it.h = s.height;
                        it.stride = (s.stride_pixels != 0) ? s.stride_pixels : (std::uint16_t)s.width;
                        it.tint = s.tint;
                        it.sort_key = sort_key;
                        sprites[sc++] = it;
                    }
                }

                // Text
                {
                    auto view = reg.view<const framedot::ecs::Transform2D,
                                        const framedot::ecs::Text2D>();
                    for (auto e : view) {
                        if (tc >= texts.size()) break;

                        const auto& t = view.get<const framedot::ecs::Transform2D>(e);
                        const auto& tx = view.get<const framedot::ecs::Text2D>(e);

                        if (tx.len == 0) continue;

                        std::uint32_t sort_key = 0;
                        if (const auto* ro = reg.try_get<framedot::ecs::RenderOrder2D>(e)) {
                            sort_key = ro->sort_key;
                        }

                        TextItem it{};
                        it.x = (int)t.position.x;
                        it.y = (int)t.position.y;
                        it.text = tx.text.data();
                        it.len = tx.len;
                        it.color = tx.color;
                        it.scale = (tx.scale == 0) ? 1 : tx.scale;
                        it.sort_key = sort_key;
                        texts[tc++] = it;
                    }
                }

                if (rc == 0 && sc == 0 && tc == 0) return;

                auto run_chunks = [&](auto& arr, std::size_t count, auto emit_one) {
                    if (count == 0) return;

                    if (ctx.jobs && ctx.jobs->worker_count() > 0) {
                        framedot::core::TaskGroup tg(ctx.jobs, framedot::core::JobLane::Engine);

                        const std::uint32_t workers = ctx.jobs->worker_count();
                        const std::size_t chunks = (workers > 0) ? (std::size_t)workers : 1;
                        const std::size_t chunk_size = (count + chunks - 1) / chunks;

                        for (std::size_t ci = 0; ci < chunks; ++ci) {
                            const std::size_t b = ci * chunk_size;
                            const std::size_t e = (b + chunk_size < count) ? (b + chunk_size) : count;
                            if (b >= e) continue;

                            tg.run([&, b, e]() noexcept {
                                for (std::size_t i = b; i < e; ++i) emit_one(arr[i]);
                            });
                        }
                        tg.wait();
                    } else {
                        for (std::size_t i = 0; i < count; ++i) emit_one(arr[i]);
                    }
                };

                // ----------------------------
                // 2) parallel emit (only RenderQueue push)
                // ----------------------------
                run_chunks(rects, rc, [&](const RectItem& it) noexcept {
                    if (it.outline) {
                        rq->rect_outline(it.x, it.y, it.w, it.h, it.outline_px, it.color, it.sort_key);
                    } else {
                        rq->fill_rect(it.x, it.y, it.w, it.h, it.color, it.sort_key);
                    }
                });

                run_chunks(sprites, sc, [&](const SpriteItem& it) noexcept {
                    rq->blit_sprite(it.x, it.y, it.pixels, it.w, it.h, it.stride, it.tint, it.sort_key);
                });

                run_chunks(texts, tc, [&](const TextItem& it) noexcept {
                    rq->text(it.x, it.y, std::string_view(it.text, it.len), it.color, it.sort_key, it.scale);
                });
            }
        );
    }

} // namespace framedot::ecs::systems
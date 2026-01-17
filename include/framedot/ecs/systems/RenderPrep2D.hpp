// include/framedot/ecs/systems/RenderPrep2D.hpp
/**
 * @file RenderPrep2D.hpp
 * @brief Basic2D 컴포넌트를 RenderQueue 커맨드로 변환하는 기본 RenderPrep 시스템.
 *
 * 철학:
 * - RenderPrep 단계는 "무엇을 그릴지"를 기록한다.
 * - 실제 픽셀 래스터는 gfx::SoftwareRenderer가 담당한다.
 *
 * 병렬화:
 * - 엔티티 목록을 고정 크기 배열에 수집한 뒤,
 *   워커가 있을 때는 chunk로 나눠 TaskGroup(Engine lane)로 병렬 처리한다.
 */
#pragma once
#include <framedot/core/Tasks.hpp>
#include <framedot/ecs/World.hpp>
#include <framedot/ecs/components/Basic2D.hpp>
#include <framedot/gfx/RenderQueue.hpp>
#include <framedot/gfx/RenderQueue.hpp>
#include <framedot/gfx/RenderQueue.hpp>

#include <array>
#include <cstdint>


namespace framedot::ecs::systems {

    /// @brief RenderPrep2D가 한 프레임에 처리할 최대 엔티티 수
    static constexpr std::size_t kMaxRenderPrepEntities = framedot::gfx::RenderQueue::kMax;

    /// @brief Basic2D(Transform2D + Rect2D)를 RenderQueue에 기록하는 기본 시스템
    inline void install_render_prep_2d(World& world) {
        world.add_read_system(Phase::RenderPrep,
            [](const framedot::core::FrameContext& ctx, const World::Registry& reg) {
                auto* rq = ctx.render_queue;
                if (!rq) return;

                // view 수집 (고정 배열)
                std::array<entt::entity, kMaxRenderPrepEntities> ents{};
                std::size_t count = 0;

                auto view = reg.view<const framedot::ecs::Transform2D,
                                    const framedot::ecs::Rect2D>();

                for (auto e : view) {
                    if (count >= ents.size()) break;
                    ents[count++] = e;
                }

                if (count == 0) return;

                auto emit_one = [&](entt::entity e) noexcept {
                    const auto* t = reg.try_get<framedot::ecs::Transform2D>(e);
                    const auto* r = reg.try_get<framedot::ecs::Rect2D>(e);
                    if (!t || !r) return;

                    // RenderOrder2D는 optional
                    std::uint32_t sort_key = 0;
                    if (const auto* ro = reg.try_get<framedot::ecs::RenderOrder2D>(e)) {
                        sort_key = ro->sort_key;
                    }

                    // 좌표는 일단 "픽셀 정수"로 내림
                    const int x = (int)t->position.x;
                    const int y = (int)t->position.y;
                    const int w = (int)r->size.x;
                    const int h = (int)r->size.y;

                    // outline_px가 있으면 테두리
                    if (r->outline_px > 0) {
                        rq->rect_outline(x, y, w, h, r->outline_px, r->color, sort_key);
                    } else {
                        // 알파가 있으면 fill_rect여도 renderer가 자동 blend 처리한다.
                        rq->fill_rect(x, y, w, h, r->color, sort_key);
                    }
                };

                // 병렬 chunk 처리
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
                            for (std::size_t i = b; i < e; ++i) emit_one(ents[i]);
                        });
                    }

                    tg.wait();
                } else {
                    for (std::size_t i = 0; i < count; ++i) emit_one(ents[i]);
                }
            }
        );
    }

} // namespace framedot::ecs::systems
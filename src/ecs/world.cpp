// src/ecs/world.cpp
/**
 * @file world.cpp
 * @brief ECS World tick 구현부. Phase 단위 실행 + ReadOnly 병렬화를 담당
 *
 * 주의:
 * - tick 내부 병렬 대기는 JobSystem 전체 idle이 아니라, 이번 phase에서 던진 일만 기다린다.
 */
#include <framedot/ecs/World.hpp>
#include <framedot/core/Tasks.hpp>


namespace framedot::ecs {

    void World::add_read_system(Phase phase, ReadSystem fn) {
        // 빈 함수면 return
        if (!fn) return;
        m_read.push_back(ReadEntry{ phase, std::move(fn) });
    }

    void World::add_write_system(Phase phase, WriteSystem fn) {
        if (!fn) return;
        m_write.push_back(WriteEntry{ phase, std::move(fn) });
    }

    void World::tick(const framedot::core::FrameContext& ctx) {
        framedot::core::JobSystem* jobs = ctx.jobs;
        const framedot::core::FrameContext* ctxp = &ctx;

        for (std::size_t pi = 0; pi < static_cast<std::size_t>(Phase::Count); ++pi) {
            const Phase p = static_cast<Phase>(pi);

            // ----------------------------
            // 1) ReadOnly 시스템: Phase 내부 병렬 후보
            // ----------------------------
            if (jobs && jobs->worker_count() > 0) {
                framedot::core::TaskGroup tg(jobs, framedot::core::JobLane::Engine);

                for (auto& e : m_read) {
                    if (e.phase != p) continue;

                    // registry는 const로만 전달
                    tg.run([ctxp, this, fn = e.fn]() {
                        fn(*ctxp, static_cast<const Registry&>(this->m_reg));
                    });
                }

                // 이번 phase에서 던진 것만 기다림 (jobs 전체 idle 아님!)
                tg.wait();
            } else {
                for (auto& e : m_read) {
                    if (e.phase != p) continue;
                    e.fn(ctx, static_cast<const Registry&>(m_reg));
                }
            }

            // ----------------------------
            // 2) Write 시스템: 기본 직렬 실행
            // ----------------------------
            for (auto& e : m_write) {
                if (e.phase != p) continue;
                e.fn(ctx, m_reg);
            }
        }
    }

} // namespace framedot::ecs
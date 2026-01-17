// src/ecs/world.cpp
#include <framedot/ecs/World.hpp>


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

        /// @brief Phase 순서대로 실행
        for (std::size_t pi = 0; pi < static_cast<std::size_t>(Phase::Count); ++pi) {
            const Phase p = static_cast<Phase>(pi);

            // ----------------------------
            // 1) ReadOnly 시스템 병렬 실행
            // ----------------------------
            bool did_enqueue = false;

            for (auto& e : m_read) {
                if (e.phase != p) continue;

                if (jobs && jobs->worker_count() > 0) {
                    // ctx는 참조 캡쳐 대신 포인터 캡쳐 (수명 명확)
                    // registry는 const로만 전달
                    jobs->enqueue([ctxp, this, fn = e.fn]() {
                        fn(*ctxp, static_cast<const Registry&>(this->m_reg));
                    });
                    did_enqueue = true;
                } else {
                    e.fn(ctx, static_cast<const Registry&>(m_reg));
                }
            }

            if (did_enqueue && jobs) {
                jobs->wait_idle();
            }

            // ----------------------------
            // 2) Write 시스템 직렬 실행
            // ----------------------------
            for (auto& e : m_write) {
                if (e.phase != p) continue;
                e.fn(ctx, m_reg);
            }
        }
    }

} // namespace framedot::ecs
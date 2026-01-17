// src/core/job_system.cpp
#include <framedot_internal/core/DefaultJobSystem.hpp>
#include <framedot/core/JobSystem.hpp>
#include <framedot/core/Config.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>


namespace framedot::core::internal {

    /// @brief std 기반 간단한 ThreadPool
    class DefaultJobSystem final : public JobSystem {
    public: 
        using JobSystem::enqueue;

        explicit DefaultJobSystem(std::uint32_t worker_threads) {
            if (framedot::core::config::enable_smp == 0u) {
                /// @brief SMP 비활성일 때는 워커를 0으로 둔다(모든 잡은 메인에서 동기 실행될 수도 있음)
                worker_threads = 0;
            }

            if (worker_threads == 0) {
                const auto hc = std::thread::hardware_concurrency();
                /// @brief 0이면 자동: (최소 1개는 두되, 메인 스레드는 제외한다는 철학)
                worker_threads = (hc > 1) ? (hc - 1) : 1;
            }

            if (worker_threads > framedot::core::config::max_worker_threads) {
                worker_threads = framedot::core::config::max_worker_threads;
            }

            m_stop = false;
            m_inflight.store(0);

            m_workers.reserve(worker_threads);
            for (std::uint32_t i = 0; i < worker_threads; ++i) {
                m_workers.emplace_back([this]() { this->worker_loop_(); });
            }
        }

        ~DefaultJobSystem() override {
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_stop = true;
            }
            m_cv.notify_all();
            for (auto& t : m_workers) {
                if (t.joinable()) t.join();
            }
        }

        std::uint32_t worker_count() const noexcept override {
            return static_cast<std::uint32_t>(m_workers.size());
        }

        void enqueue(JobLane lane, Job job) override {
            if (!job) return;

            if (m_workers.empty()) {
                /// @brief 워커가 없으면(싱글스레드 모드) 즉시 실행
                job();
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_mtx);

                /// @brief 엔진 lane 우선 처리: 큐를 분리
                if (lane == JobLane::Engine) {
                    m_jobs_engine.push(std::move(job));
                } else {
                    m_jobs_user.push(std::move(job));
                }

                m_inflight.fetch_add(1, std::memory_order_relaxed);
            }

            m_cv.notify_one();
        }


        void wait_idle() override {
            /// @brief 현재 inflight가 0이 될 때까지 대기
            std::unique_lock<std::mutex> lock(m_idle_mtx);
            m_idle_cv.wait(lock, [this]() { return m_inflight.load() == 0; });
        }

    private:
        void worker_loop_() {
            while (true) {
                Job job;
                {
                    std::unique_lock<std::mutex> lock(m_mtx);
                    m_cv.wait(lock, [this]() {
                        return m_stop || !m_jobs_engine.empty() || !m_jobs_user.empty();
                    });

                    if (m_stop && m_jobs_engine.empty() && m_jobs_user.empty()) {
                        return;
                    }

                    // Engine lane을 먼저 소비해서 프레임 안정성/latency를 우선
                    if (!m_jobs_engine.empty()) {
                        job = std::move(m_jobs_engine.front());
                        m_jobs_engine.pop();
                    } else {
                        job = std::move(m_jobs_user.front());
                        m_jobs_user.pop();
                    }
                }

                /// @brief 잡 실행
                job();

                /// @brief 완료 카운트 감소 및 idle notify
                const auto left = m_inflight.fetch_sub(1) - 1;
                if (left == 0) {
                    std::lock_guard<std::mutex> lk(m_idle_mtx);
                    m_idle_cv.notify_all();
                }
            }
        }

        std::vector<std::thread> m_workers;
        std::mutex m_mtx;
        std::condition_variable m_cv;

        std::queue<Job> m_jobs_engine;
        std::queue<Job> m_jobs_user;
        bool m_stop{false};

        std::atomic<std::uint32_t> m_inflight{0};

        std::mutex m_idle_mtx;
        std::condition_variable m_idle_cv;
    };

    JobSystem* create_default_jobsystem(std::uint32_t worker_threads) {
        return new DefaultJobSystem(worker_threads);
    }

    void destroy_default_jobsystem(JobSystem* js) noexcept {
        delete js;
    }

} // namespace framedot::core::internal
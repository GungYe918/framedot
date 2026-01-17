// include/framedot/core/Tasks.hpp
#pragma once
#include <framedot/core/JobSystem.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>


namespace framedot::core {

    /// @brief RAII 기반 태스크 그룹
    /// - scope를 벗어나면 자동으로 wait
    /// - 유저 lane / 엔진 lane 모두 지원하지만, 유저 편의 API로는 User 기본 추천
    class TaskGroup {
    public:
        explicit TaskGroup(JobSystem* js, JobLane lane = JobLane::User) noexcept 
            : m_js(js), m_lane(lane) {}

        TaskGroup(const TaskGroup&) = delete;
        TaskGroup& operator=(const TaskGroup&) = delete;

        ~TaskGroup() { wait(); }

        /// @brief 현재 워커가 존재하는지
        bool parallel_ok() const noexcept {
            return m_js && (m_js->worker_count() > 0);
        }

        /// @brief std::function 전용: 비어있으면 무시
        void run(JobSystem::Job job) {
            if (!job) return;

            if (!parallel_ok()) {
                job();
                return;
            }

            m_inflight.fetch_add(1, std::memory_order_relaxed);

            m_js->enqueue(m_lane, [this, j = std::move(job)]() mutable {
                j();
                done_one_();
            });
        }

        /// @brief 일반 callable(람다 등) 실행
        template <class F>
        void run(F&& fn) {
            if (!parallel_ok()) {
                fn();
                return;
            }

            m_inflight.fetch_add(1, std::memory_order_relaxed);

            m_js->enqueue(m_lane, [this, f = std::forward<F>(fn)]() mutable {
                f();
                done_one_();
            });
        }

        /// @brief 그룹 내 태스크가 모두 끝날 때까지 대기
        void wait() noexcept {
            if (!m_js) return;

            // inflight가 0이면 빠르게 리턴
            if (m_inflight.load(std::memory_order_acquire) == 0) return;

            std::unique_lock<std::mutex> lock(m_mtx);
            m_cv.wait(lock, [this]() {
                return m_inflight.load(std::memory_order_acquire) == 0;
            });
        }
    
    private:
        void done_one_() {
            const auto left = m_inflight.fetch_sub(1, std::memory_order_acq_rel) - 1;
            if (left == 0) {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_cv.notify_all();
            }
        }

        JobSystem* m_js{nullptr};
        JobLane    m_lane{JobLane::User};

        std::atomic<std::uint32_t> m_inflight{0};
        std::mutex m_mtx;
        std::condition_variable m_cv;
    };

    /// @brief 태스크 결과 컨테이너 (할당 없이 결과 받기)
    /// - TaskGroup과 함께 쓰는 것을 전제
    template <typename T>
    class TaskValue {
    public:
        /// @brief 결과가 준비되었는지
        bool ready() const noexcept {  return m_ready.load(std::memory_order_acquire);  }

        /// @brief 결과 접근
        const T& get() const noexcept { return *m_value; }
        T& get() noexcept { return *m_value; }
    
        /// @brief 내부용: 태스크에서 결과를 세팅
        void set(T v) noexcept(std::is_nothrow_move_constructible_v<T>) {
            m_value = std::move(v);
            m_ready.store(true, std::memory_order_release);
        }

    private:
        std::atomic<bool> m_ready{false};
        std::optional<T>  m_value;
    };

    /// @brief void 특수화
    template <>
    class TaskValue<void> {
    public:
        bool ready() const noexcept { return m_ready.load(std::memory_order_acquire); }
        void set() noexcept { m_ready.store(true, std::memory_order_release); }
    private:
        std::atomic<bool> m_ready{false};
    };

    /// @brief 태스크를 실행하고 결과를 TaskValue로 돌려받는 편의 함수
    template <class F, class R = std::invoke_result_t<F>>
    void run_value(TaskGroup& tg, TaskValue<R>& out, F&& fn) {
        if constexpr (std::is_void_v<R>) {
            tg.run([&out, f = std::forward<F>(fn)]() mutable {
                f();
                out.set();
            });
        } else {
            tg.run([&out, f = std::forward<F>(fn)]() mutable {
                out.set(f());
            });
        }
    }
    
} // namespace framedot::core
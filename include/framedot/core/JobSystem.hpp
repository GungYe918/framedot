// include/framedot/core/JobSystem.hpp
#pragma once
#include <cstdint>
#include <functional>


namespace framedot::core {

    /// @brief 잡 우선순위/성격을 구분하기 위한 lane
    /// - Engine: 엔진이 자동으로 쌓는 작업(프레임 안정성/latency 우선)
    /// - User: 유저가 명시적으로 제출한 작업
    enum class JobLane : std::uint8_t {
        Engine = 0,
        User   = 1,
    };

    /// @brief 멀티스레드 작업 실행을 위한 간단한 잡 시스템 인터페이스
    /// - 게임 루프에서 비싼 계산을 워커 스레드로 분산하기 위한 최소 기능만 제공
    /// - 렌더링 present/ncurses 같은 플랫폼 의존 작업은 메인 스레드에서 처리 권장
    class JobSystem {
    public:
        using Job = std::function<void()>;

        JobSystem() = default;
        virtual ~JobSystem() = default;

        JobSystem(const JobSystem&) = delete;
        JobSystem& operator=(const JobSystem&) = delete;

        /// @brief 워커 스레드 개수
        virtual std::uint32_t worker_count() const noexcept = 0;

        /// @brief job enqueue (lane 지정)
        virtual void enqueue(JobLane lane, Job job) = 0;

        /// @brief 기본 lane enqueue
        void enqueue(Job job) { enqueue(JobLane::Engine, std::move(job)); }

        /// @brief 지금까지 enqueue된 잡이 전부 끝날 때까지 대기
        virtual void wait_idle() = 0;
    };

    /// @brief 기본 구현 생성 헬퍼
    JobSystem* create_default_jobsystem(std::uint32_t worker_threads);

    /// @brief 기본 구현 파괴 헬퍼
    void destroy_default_jobsystem(JobSystem* js) noexcept;

} // namespace framedot::core
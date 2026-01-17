// internal/framedot_internal/core/DefaultJobSystem.hpp
#pragma once
#include <framedot/core/JobSystem.hpp>
#include <cstdint>


namespace framedot::core::internal {

    /// @brief 기본 구현 생성 헬퍼
    JobSystem* create_default_jobsystem(std::uint32_t worker_threads);

    /// @brief 기본 구현 파괴 헬퍼
    void destroy_default_jobsystem(JobSystem* js) noexcept;

} // namespace framedot::core::internal
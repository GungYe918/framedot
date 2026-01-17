// include/framedot/ecs/World.hpp
/**
 * @file World.hpp
 * @brief EnTT registry를 소유하고, 시스템을 Phase 순서대로 실행하는 ECS World
 *
 */
#pragma once
#include <entt/entt.hpp>

#include <framedot/core/FrameContext.hpp>
#include <framedot/core/JobSystem.hpp>

#include <cstdint>
#include <functional>
#include <vector>


namespace framedot::ecs {

    /// @brief ECS 업데이트 단계를 구분하는 간단한 Phase
    /// - 같은 Phase 내에서 읽기 전용 시스템은 병렬 실행 가능
    enum class Phase : std::uint8_t {
        PreUpdate = 0,
        Update,
        PostUpdate,
        RenderPrep,
        Count
    };

    /// @brief framedot ECS World
    /// - EnTT registry를 소유
    /// - 시스템을 등록하고, 프레임마다 tick()으로 실행
    /// - 1차 SMP: ReadOnly 시스템만 병렬 실행
    class World {
    public:
        using Registry = entt::registry;

        /// @brief 쓰기 가능한 시스템(직렬 실행)
        using WriteSystem = std::function<void(const framedot::core::FrameContext&, Registry&)>;

        /// @brief 읽기 전용 시스템(병렬 실행 가능)
        using ReadSystem  = std::function<void(const framedot::core::FrameContext&, const Registry&)>;

        /// @brief registry 접근 (엔진 내부 entity 생성/삭제 등)
        Registry& registry() noexcept {  return m_reg;  }

        /// @brief registry 접근(읽기 전용)
        const Registry& registry() const noexcept { return m_reg; }

        /// @brief 읽기 전용 시스템 등록
        /// - 같은 Phase의 ReadOnly 시스템은 job system으로 병렬 실행된다(가능할 때).
        void add_read_system(Phase phase, ReadSystem fn);

        /// @brief 쓰기 시스템 등록
        /// - registry를 변경할 수 있으므로 기본적으로 직렬 실행된다.
        void add_write_system(Phase phase, WriteSystem fn);

        /// @brief 프레임 업데이트
        /// - Phase 순서대로 실행
        /// - Phase 내부: (ReadOnly 병렬) -> (Write 직렬)
        void tick(const framedot::core::FrameContext& ctx);

    private:
        struct ReadEntry {
            Phase phase;
            ReadSystem fn;
        };

        struct WriteEntry {
            Phase phase;
            WriteSystem fn;
        };

        static constexpr std::size_t phase_index_(Phase p) noexcept {
            return static_cast<std::size_t>(p);
        }

        Registry m_reg;
        
        /// @brief 읽기 전용 시스템들(병렬 후보)
        std::vector<ReadEntry>  m_read;

        /// @brief 쓰기 시스템들(직렬)
        std::vector<WriteEntry> m_write;
    };

} // namespace framedot::ecs
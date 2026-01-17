// include/framedot/ecs/Fecs.hpp
/**
 * @file Fecs.hpp
 * @brief framedot ECS 퍼블릭 헤더
 *
 * 목표:
 * - 유저는 이 파일만 include하여 World/Phase/컴포넌트/시스템 설치
 * - 내부 구현이 EnTT여도 외부 네임스페이스는 fecs로 고정.
 * - 나중에 EnTT 교체 시, 여기의 using/래퍼를 바꿔서 충격을 최소화.
 */
#pragma once

// core ECS
#include <framedot/ecs/World.hpp>

// components
#include <framedot/ecs/components/Basic2D.hpp>

// systems
#include <framedot/ecs/systems/RenderPrep2D.hpp>

namespace framedot::fecs {

    // ---- World / Phase ----
    using Phase = framedot::ecs::Phase;
    using World = framedot::ecs::World;
    using Registry = framedot::ecs::World::Registry;

    // ---- Entity 타입도 여기서 통일해서 쓰게 만든다(현재는 EnTT) ----
    using Entity = entt::entity;
    static constexpr Entity null = entt::null;

    // ---- 기본 컴포넌트 re-export ----
    using Transform2D = framedot::ecs::Transform2D;
    using Rect2D      = framedot::ecs::Rect2D;
    using RenderOrder2D = framedot::ecs::RenderOrder2D;

    using Sprite2D = framedot::ecs::Sprite2D;
    using Text2D   = framedot::ecs::Text2D;

    // ---- 시스템 네임스페이스도 fecs::systems 로 제공 ----
    namespace systems {
        using framedot::ecs::systems::install_render_prep_2d;
    }

} // namespace framedot::fecs
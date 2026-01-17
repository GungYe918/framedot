// include/framedot/ecs/Fecs.hpp
/**
 * @file Fecs.hpp
 * @brief framedot ECS 퍼블릭 타입 레이어.
 *
 * 목표:
 * - 외부(게임 코드)에서는 EnTT 타입을 직접 보지 않게 한다.
 * - 지금은 내부 구현이 EnTT지만, 나중에 교체(EnTT 제거) 가능성을 남긴다.
 *
 * 사용 예:
 *   framedot::fecs::World world;
 *   auto e = world.registry().create();
 */
#pragma once
#include <framedot/ecs/World.hpp>


namespace framedot::fecs {

    using Phase = framedot::ecs::Phase;
    using World = framedot::ecs::World;

    using Registry = framedot::ecs::World::Registry;

} // namespace framedot::fecs

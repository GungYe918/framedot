// framedot/ecs/components/Basic2D.hpp
#pragma once
#include <framedot/math/Types.hpp>
#include <framedot/gfx/Color.hpp>

#include <cstdint>


namespace framedot::ecs {

    /// @brief 로컬 2D 트랜스폼
    struct Transform2D {
        framedot::math::Vec2f position  {0.0f, 0.0f};
        framedot::math::Vec2f scale     {1.0f, 1.0f};

        /// @brief 라디안 회전(반시계)
        float rotation_rad {0.0f};
    };

    /// @brief 단순 이동 (속도)
    struct Velocity2D {
        framedot::math::Vec2f v {0.0f, 0.0f};
    };

    /// @brief 렌더링 정렬 키
    /// - 값이 작을수록 먼저 그려짐 (뒤 배경 -> 앞 오브젝트)
    struct RenderOrder2D {
        std::uint32_t sort_key {0};
    };

    /// @brief 사각형 (Shape)
    /// - RenderPrep에서 RenderQueue로 변환되는 그릴 대상
    struct Rect2D {
        framedot::math::Vec2f size      {8.0f, 8.0f};
        framedot::gfx::ColorRGBA8 color {255, 255, 255, 255};

        /// @brief 0이면 채우기, 1 이상이면 테두리 두께 (픽셀)
        std::uint16_t outline_px{0};

        /// @brief 0이면 불투명
        std::uint8_t  _pad0{0};
    };

} // namespace framedot::ecs
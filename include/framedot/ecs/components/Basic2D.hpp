// framedot/ecs/components/Basic2D.hpp
#pragma once
#include <framedot/math/Types.hpp>
#include <framedot/gfx/Color.hpp>

#include <cstdint>
#include <array>


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

    // 렌더링 정렬 키
    // - 값이 작을수록 먼저 그려짐 (뒤 배경 -> 앞 오브젝트)
    struct RenderOrder2D {
        std::uint32_t sort_key {0};
    };

    /// 사각형 (Shape)
    /// - RenderPrep에서 RenderQueue로 변환되는 그릴 대상
    struct Rect2D {
        framedot::math::Vec2f size      {8.0f, 8.0f};
        framedot::gfx::ColorRGBA8 color {255, 255, 255, 255};

        /// @brief 0이면 채우기, 1 이상이면 테두리 두께 (픽셀)
        std::uint16_t outline_px{0};

        /// @brief 0이면 불투명
        std::uint8_t  _pad0{0};
    };

    // 외부 픽셀 버퍼를 참조하는 간단 스프라이트
    // - pixels는 RGBA8888(0xRRGGBBAA)
    // - stride_pixels >= width
    struct Sprite2D {
        const std::uint32_t* pixels{nullptr};
        std::int32_t width{0};
        std::int32_t height{0};
        std::uint16_t stride_pixels{0};

        // tint (곱) + alpha
        framedot::gfx::ColorRGBA8 tint{255, 255, 255, 255};
    };

    /// @brief per-entity 고정 텍스트 (프레임마다 바뀌어도 할당 없음)
    struct Text2D {
        static constexpr std::size_t kMax = 96;

        std::array<char, kMax> text{};
        std::uint16_t len{0};

        framedot::gfx::ColorRGBA8 color{255, 255, 255, 255};

        // 단순 스케일(나중에 폰트 들어오면 의미 바뀔 수 있음)
        std::uint8_t scale{1};
    };

} // namespace framedot::ecs
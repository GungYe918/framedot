// include framedot/gfx/Color.hpp
#pragma once
#include <cstdint>

namespace framedot::gfx {

    struct ColorRGBA8 {
        std::uint8_t r, g, b, a;
    };

    class Color {
    public:
        static constexpr ColorRGBA8 rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) noexcept {
            return ColorRGBA8{r, g, b, a};
        }
    };

} // namespace framedot::gfx
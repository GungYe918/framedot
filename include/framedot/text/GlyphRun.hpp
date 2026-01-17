// include/framedot/text/GlyphRun.hpp
#pragma once
#include <cstdint>
#include <vector>


namespace framedot::text {

    struct Glyph {
        std::uint32_t glyph_id{0};
        std::int32_t  x_advance{0};
        std::int32_t  y_advance{0};
        std::int32_t  x_offset{0};
        std::int32_t  y_offset{0};
        std::uint32_t cluster{0};
    };

    struct GlyphRun {
        std::vector<Glyph> glyphs;
    };

} // namespace framedot::text
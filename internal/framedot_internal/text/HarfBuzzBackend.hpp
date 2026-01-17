// internal/framedot_internal/text/HarfBuzzBackend.hpp
#pragma once
#include <framedot/text/TextTypes.hpp>
#include <framedot/text/GlyphRun.hpp>

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>


namespace framedot::text::internal {

    class HarfBuzzBackend {
    public:
        HarfBuzzBackend();
        ~HarfBuzzBackend();

        FontHandle add_font_from_memory(std::span<const std::uint8_t> font_data);

        GlyphRun shape_utf8(
            FontHandle font,
            std::string_view utf8,
            std::uint32_t px_size,
            Direction dir
        ) const;    

    private:
        struct FontEntry;
        std::vector<FontEntry> m_fonts;
    };

} // namespace framedot::text::internal

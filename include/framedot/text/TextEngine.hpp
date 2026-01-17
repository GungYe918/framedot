// include/framedot/text/TextEngine.hpp
#pragma once
#include <framedot/text/TextTypes.hpp>
#include <framedot/text/GlyphRun.hpp>

#include <string_view>
#include <span>
#include <cstdint>


namespace framedot::text {

    class TextEngine {
    public:
        TextEngine();
        ~TextEngine();

        TextEngine(const TextEngine&) = delete;
        TextEngine& operator=(const TextEngine&) = delete;

        // 폰트 등록: 엔진이 데이터 복사/보관(수명 안전)
        FontHandle add_font_from_memory(std::span<const std::uint8_t> font_data);

        // shaping
        GlyphRun shape_utf8(
            FontHandle font,
            std::string_view utf8,
            std::uint32_t px_size,
            Direction dir = Direction::LTR
        ) const;

    private:
        struct Impl;
        Impl* m_impl;
    };

} // namespace framedot::text
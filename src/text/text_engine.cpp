// src/text/text_engine.cpp
#include <framedot/text/TextEngine.hpp>
#include <framedot_internal/text/HarfBuzzBackend.hpp>


namespace framedot::text {

    struct TextEngine::Impl {
        internal::HarfBuzzBackend hb;
    };

    TextEngine::TextEngine() 
        : m_impl(new Impl{})  {  }
    
    TextEngine::~TextEngine() {  delete m_impl;  }

    FontHandle TextEngine::add_font_from_memory(std::span<const std::uint8_t> font_data) {
        return m_impl->hb.add_font_from_memory(font_data);
    }

    GlyphRun TextEngine::shape_utf8(
        FontHandle font,
        std::string_view utf8,
        std::uint32_t px_size,
        Direction dir
    ) const {
        return m_impl->hb.shape_utf8(font, utf8, px_size, dir);
    }

} // namespace framedot::text
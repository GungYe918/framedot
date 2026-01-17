// src/text/harfbuzz_backend.cpp
#include <framedot_internal/text/HarfBuzzBackend.hpp>

#include <hb.h>
#include <hb-ot.h>

#include <cstring>


namespace framedot::text::internal {

    struct HarfBuzzBackend::FontEntry {
        hb_blob_t* blob{nullptr};
        hb_face_t* face{nullptr};
        hb_font_t* font{nullptr};
    };

    HarfBuzzBackend::HarfBuzzBackend() = default;

    HarfBuzzBackend::~HarfBuzzBackend() {
        for (auto& f : m_fonts) {
            if (f.font) hb_font_destroy(f.font);
            if (f.face) hb_face_destroy(f.face);
            if (f.blob) hb_blob_destroy(f.blob);
        }
    }

    FontHandle HarfBuzzBackend::add_font_from_memory(std::span<const std::uint8_t> font_data) {
        if (font_data.empty()) return {};

        // HB가 blob을 복사헤서 가져감
        hb_blob_t* blob = hb_blob_create(
            reinterpret_cast<const char*>(font_data.data()),
            (unsigned)font_data.size(),
            HB_MEMORY_MODE_DUPLICATE,
            nullptr, nullptr
        );
        hb_face_t* face = hb_face_create(blob, 0);
        hb_font_t* font = hb_font_create(face);

        // OpenType 폰트 함수 세팅
        hb_ot_font_set_funcs(font);

        FontEntry e{};
        e.blob = blob;
        e.face = face;
        e.font = font;

        m_fonts.push_back(e);
        const std::uint32_t id = (std::uint32_t)m_fonts.size(); // 1-based
        return FontHandle{id};
    }

    GlyphRun HarfBuzzBackend::shape_utf8(
        FontHandle font_h,
        std::string_view utf8,
        std::uint32_t px_size,
        Direction dir
    ) const {
        GlyphRun out{};
        if (font_h.id == 0 || font_h.id > m_fonts.size()) return out;
        if (utf8.empty()) return out;

        const FontEntry& fe = m_fonts[font_h.id - 1];
        hb_font_t* font = fe.font;

        // scale: HarfBuzz는 보통 26.6 fixed를 씀
        const int scale = (int)px_size * 64;
        hb_font_set_scale(font, scale, scale);

        hb_buffer_t* buf = hb_buffer_create();
        hb_buffer_add_utf8(buf, utf8.data(), (int)utf8.size(), 0, (int)utf8.size());

        hb_direction_t hbdir = (dir == Direction::RTL) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;
        hb_buffer_set_direction(buf, hbdir);

        // script/language는 일단 guess (추후 API로 노출 가능)
        hb_buffer_guess_segment_properties(buf);

        hb_shape(font, buf, nullptr, 0);

        unsigned int glyph_count = 0;
        hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buf, &glyph_count);
        hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

        out.glyphs.reserve(glyph_count);
        for (unsigned int i = 0; i < glyph_count; ++i) {
            Glyph g{};
            g.glyph_id = infos[i].codepoint;
            g.cluster  = infos[i].cluster;
            g.x_advance = pos[i].x_advance;
            g.y_advance = pos[i].y_advance;
            g.x_offset  = pos[i].x_offset;
            g.y_offset  = pos[i].y_offset;
            out.glyphs.push_back(g);
        }

        hb_buffer_destroy(buf);
        return out;
    }

} // namespace framedot::text::internal
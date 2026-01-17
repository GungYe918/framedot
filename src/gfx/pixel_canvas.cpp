// src/gfx/pixel_canvas.cpp
/**
 * @file pixel_canvas.cpp
 * @brief PixelCanvas의 구현부.
 */
#include <framedot/gfx/PixelCanvas.hpp>


namespace framedot::gfx {

    static inline std::uint32_t u8(std::uint8_t v) { return static_cast<std::uint32_t>(v); }

    PixelCanvas::PixelCanvas(std::uint32_t w, std::uint32_t h) {
        resize(w, h);
    }

    void PixelCanvas::resize(std::uint32_t w, std::uint32_t h) {
        m_w = w;
        m_h = h;
        m_pixels.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0u);
    }

    PixelCanvas::Pixel PixelCanvas::pack(ColorRGBA8 c) noexcept {
        // RGBA8888 packed in 0xRRGGBBAA
        return (u8(c.r) << 24) | (u8(c.g) << 16) | (u8(c.b) << 8) | (u8(c.a));
    }

    void PixelCanvas::clear(ColorRGBA8 c) noexcept {
        const Pixel p = pack(c);
        for (auto& px : m_pixels) px = p;
    }

    void PixelCanvas::put_pixel(std::int32_t x, std::int32_t y, ColorRGBA8 c) noexcept {
        if (x < 0 || y < 0) return;
        if (static_cast<std::uint32_t>(x) >= m_w) return;
        if (static_cast<std::uint32_t>(y) >= m_h) return;
        m_pixels[static_cast<std::size_t>(y) * m_w + static_cast<std::size_t>(x)] = pack(c);
    }

} // namespace framedot::gfx
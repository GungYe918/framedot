// include/framedot/gfx/PixelCanvas.hpp
#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <cassert>
#include <framedot/gfx/Color.hpp>


namespace framedot::gfx {

    class PixelCanvas {
    public:
        using Pixel = std::uint32_t;

        PixelCanvas() = default;
        PixelCanvas(std::uint32_t w, std::uint32_t h);

        void resize(std::uint32_t w, std::uint32_t h);

        std::uint32_t width()  const noexcept { return m_w; }
        std::uint32_t height() const noexcept { return m_h; }

        void clear(ColorRGBA8 c) noexcept;
        void put_pixel(std::int32_t x, std::int32_t y, ColorRGBA8 c) noexcept;

        std::span<Pixel>       pixels() noexcept { return m_pixels; }
        std::span<const Pixel> pixels() const noexcept { return m_pixels; }

        static Pixel pack(ColorRGBA8 c) noexcept;

    private:
        std::uint32_t m_w{0}, m_h{0};
        std::vector<Pixel> m_pixels;
    };

} // namespace framedot::gfx
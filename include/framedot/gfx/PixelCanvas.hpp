// include/framedot/gfx/PixelCanvas.hpp
/**
 * @file PixelCanvas.hpp
 * @brief 엔진 내부 소프트웨어 픽셀 버퍼(소유)와 기본 픽셀 연산 API를 제공한다.
 *
 * 출력 어댑터에는 PixelCanvas 자체를 넘기지 않고, PixelFrame(view)을 넘긴다.
 */
#pragma once
#include <framedot/gfx/Color.hpp>
#include <framedot/gfx/PixelFrame.hpp>

#include <cstdint>
#include <vector>
#include <span>
#include <cassert>


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

        /// @brief 출력 어댑터용 불변 프레임 뷰 
        PixelFrame frame() const noexcept {
            PixelFrame f{};
            f.width = m_w;
            f.height = m_h;
            f.stride_pixels = m_w;
            f.format = PixelFormat::RGBA8888;
            f.pixels = std::span<const std::uint32_t>(m_pixels.data(), m_pixels.size());
            return f;
        }

    private:
        std::uint32_t m_w{0}, m_h{0};
        std::vector<Pixel> m_pixels;
    };

} // namespace framedot::gfx
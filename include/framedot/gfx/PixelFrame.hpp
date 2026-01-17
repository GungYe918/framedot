// include/framedot/gfx/PixelFrame.hpp
/**
 * @file PixelFrame.hpp
 * @brief PixelCanvas 등에서 생성되는 "불변(immutable) 픽셀 프레임 뷰"를 정의한다.
 *
 * 기본 출력 경로는 serialize가 아니라 zero-copy view(PixelFrame)를 사용한다.
 * serialize는 캡처/IPC/네트워크 전송 같은 특수 케이스에서만 사용 권장.
 */
#pragma once
#include <cstdint>
#include <span>
#include <vector>


namespace framedot::gfx {

    enum class PixelFormat : std::uint8_t {
        RGBA8888 = 0, // 0xRRGGBBAA (현재 PixelCanvas 규약)
    };

    class PixelFrame {
    public:
        std::uint32_t width{0};
        std::uint32_t height{0};
        std::uint32_t stride_pixels{0};
        PixelFormat   format{PixelFormat::RGBA8888};
    
        std::span<const std::uint32_t> pixels; // 불변 뷰

        bool valid() const noexcept {
            return width > 0 && height > 0 && stride_pixels >= width &&
                   !pixels.empty();
        }

        static constexpr std::uint8_t r(std::uint32_t p) noexcept { return (p >> 24) & 0xFF; }
        static constexpr std::uint8_t g(std::uint32_t p) noexcept { return (p >> 16) & 0xFF; }
        static constexpr std::uint8_t b(std::uint32_t p) noexcept { return (p >>  8) & 0xFF; }
        static constexpr std::uint8_t a(std::uint32_t p) noexcept { return (p >>  0) & 0xFF; }

        /**
         * @brief serialize (옵션): RGBA8888 raw bytes로 덤프한다.
         * @note 기본 출력 경로는 serialize가 아니라 PixelFrame view를 사용하라.
         */
        std::vector<std::uint8_t> serialize_rgba8888() const {
            std::vector<std::uint8_t> out;
            if (!valid()) return out;

            out.resize(static_cast<std::size_t>(width) * height * 4);
            std::size_t di = 0;

            // stride 고려
            for (std::uint32_t y = 0; y < height; ++y) {
                const std::size_t row = static_cast<std::size_t>(y) * stride_pixels;
                for (std::uint32_t x = 0; x < width; ++x) {
                    const std::uint32_t p = pixels[row + x];
                    out[di++] = r(p);
                    out[di++] = g(p);
                    out[di++] = b(p);
                    out[di++] = a(p);
                }
            }
            return out;
        }
    };

} // namespace framedot::gfx
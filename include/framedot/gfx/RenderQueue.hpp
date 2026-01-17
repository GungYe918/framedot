// include/framedot/gfx/RenderQueue.hpp
/**
 * @file RenderQueue.hpp
 * @brief RenderPrep 단계에서 "무엇을 그릴지"를 기록하는 고정 용량 커맨드 큐.
 *
 * World/ECS는 픽셀 계산을 직접 하지 않는다.
 * (픽셀 규칙은 SoftwareRenderer 등 gfx 서브시스템이 담당)
 */
#pragma once
#include <framedot/gfx/Color.hpp>

#include <cstdint>
#include <array>


namespace framedot::gfx {

    class RenderQueue {
    public:
        enum class Op : std::uint8_t {
            Clear = 0,
            PutPixel,
            FillRect,
            HLine,
            VLine,
        };

        struct Cmd {
            Op op{};
            ColorRGBA8 color{0,0,0,255};

            // union 흉내: 단순 POD 필드로 고정
            std::int32_t x0{0}, y0{0};
            std::int32_t x1{0}, y1{0}; // rect: (x0,y0,w=x1,h=y1) / line: (x0,y0)-(x1,y1)
        };

        static constexpr std::size_t kMax = 8192;

        void begin_frame() noexcept         {  m_count = 0;          }
        std::size_t size() const noexcept   {  return m_count;       }
        const Cmd* data() const noexcept    {  return m_cmds.data(); }

        bool clear(ColorRGBA8 c) noexcept { return push_({Op::Clear, c, 0,0,0,0}); }
        bool put_pixel(std::int32_t x, std::int32_t y, ColorRGBA8 c) noexcept {
            return push_({Op::PutPixel, c, x,y,0,0});
        }

        bool fill_rect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, ColorRGBA8 c) noexcept {
            return push_({Op::FillRect, c, x,y,w,h});
        }

        bool hline(std::int32_t x0, std::int32_t x1, std::int32_t y, ColorRGBA8 c) noexcept {
            return push_({Op::HLine, c, x0,y,x1,0});
        }
        
        bool vline(std::int32_t x, std::int32_t y0, std::int32_t y1, ColorRGBA8 c) noexcept {
            return push_({Op::VLine, c, x,y0,0,y1});
        }


    private:
        bool push_(const Cmd& c) noexcept {
            if (m_count >= kMax) return false;
            m_cmds[m_count++] = c;
            return true;
        }

        std::array<Cmd, kMax> m_cmds{};
        std::size_t m_count{0};
    };

} // namespace framedot::gfx
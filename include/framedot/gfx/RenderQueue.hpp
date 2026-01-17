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
#include <atomic>


namespace framedot::gfx {

    class RenderQueue {
    public:
        enum class Op : std::uint8_t {
            Clear = 0,
            PutPixel,
            FillRect,
            RectOutline,
            Line,
            HLine,
            VLine,
            BlendRect,   // 알파 블렌딩 사각형
            FillCircle,
            Circle,
        };

        struct Cmd {
            Op op{};
            ColorRGBA8 color{0, 0, 0, 255};

            /// @brief 정렬 키(작을수록 먼저 실행)
            std::uint32_t sort_key{0};

            // - rect: (x0,y0,w=x1,h=y1)
            // - line: (x0,y0)-(x1,y1)
            // - circle: center=(x0,y0), radius=x1
            std::int32_t x0{0}, y0{0};
            std::int32_t x1{0}, y1{0};

            /// @brief 옵션 필드(두께/플래그 등 확장용)
            std::uint16_t u0{0};
            std::uint16_t u1{0};
        };

        static constexpr std::size_t kMax = 8192;

        void begin_frame() noexcept {
            m_count.store(0, std::memory_order_release);
            m_dropped.store(0, std::memory_order_release);
        }

        std::size_t size() const noexcept {
            const std::uint32_t c = m_count.load(std::memory_order_acquire);
            return (c < static_cast<std::uint32_t>(kMax)) ? static_cast<std::size_t>(c) : kMax;
        }

        const Cmd* data() const noexcept { return m_cmds.data(); }

        /// @brief queue overflow로 드랍된 커맨드 수
        std::uint32_t dropped() const noexcept {
            return m_dropped.load(std::memory_order_acquire);
        }

        // ----------------------------
        // API
        // ----------------------------

        bool clear(ColorRGBA8 c) noexcept {
            return push_({
                Op::Clear, c, 0, 0, 0, 0  });
        }

        bool put_pixel(std::int32_t x, std::int32_t y, ColorRGBA8 c, std::uint32_t sort_key = 0) noexcept {
            Cmd cmd{};
            cmd.op = Op::PutPixel;
            cmd.color = c;
            cmd.sort_key = sort_key;
            cmd.x0 = x; cmd.y0 = y;
            return push_(cmd);
        }

        bool fill_rect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h,
                       ColorRGBA8 c, std::uint32_t sort_key = 0) noexcept {
            Cmd cmd{};
            cmd.op = Op::FillRect;
            cmd.color = c;
            cmd.sort_key = sort_key;
            cmd.x0 = x; cmd.y0 = y; cmd.x1 = w; cmd.y1 = h;
            return push_(cmd);
        }

        bool rect_outline(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h,
                          std::uint16_t thickness_px,
                          ColorRGBA8 c, std::uint32_t sort_key = 0) noexcept {
            Cmd cmd{};
            cmd.op = Op::RectOutline;
            cmd.color = c;
            cmd.sort_key = sort_key;
            cmd.x0 = x; cmd.y0 = y; cmd.x1 = w; cmd.y1 = h;
            cmd.u0 = thickness_px;
            return push_(cmd);
        }

        bool line(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1,
                  ColorRGBA8 c, std::uint32_t sort_key = 0) noexcept {
            Cmd cmd{};
            cmd.op = Op::Line;
            cmd.color = c;
            cmd.sort_key = sort_key;
            cmd.x0 = x0; cmd.y0 = y0; cmd.x1 = x1; cmd.y1 = y1;
            return push_(cmd);
        }

        bool blend_rect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h,
                        ColorRGBA8 c, std::uint32_t sort_key = 0) noexcept {
            Cmd cmd{};
            cmd.op = Op::BlendRect;
            cmd.color = c;
            cmd.sort_key = sort_key;
            cmd.x0 = x; cmd.y0 = y; cmd.x1 = w; cmd.y1 = h;
            return push_(cmd);
        }

        bool fill_circle(std::int32_t cx, std::int32_t cy, std::int32_t radius,
                         ColorRGBA8 c, std::uint32_t sort_key = 0) noexcept {
            Cmd cmd{};
            cmd.op = Op::FillCircle;
            cmd.color = c;
            cmd.sort_key = sort_key;
            cmd.x0 = cx; cmd.y0 = cy; cmd.x1 = radius;
            return push_(cmd);
        }

        bool circle(std::int32_t cx, std::int32_t cy, std::int32_t radius,
                    ColorRGBA8 c, std::uint32_t sort_key = 0) noexcept {
            Cmd cmd{};
            cmd.op = Op::Circle;
            cmd.color = c;
            cmd.sort_key = sort_key;
            cmd.x0 = cx; cmd.y0 = cy; cmd.x1 = radius;
            return push_(cmd);
        }

        bool hline(std::int32_t x0, std::int32_t x1, std::int32_t y, ColorRGBA8 c, std::uint32_t sort_key = 0) noexcept {
            Cmd cmd{};
            cmd.op = Op::HLine;
            cmd.color = c;
            cmd.sort_key = sort_key;
            cmd.x0 = x0; cmd.y0 = y; cmd.x1 = x1;
            return push_(cmd);
        }

        bool vline(std::int32_t x, std::int32_t y0, std::int32_t y1, ColorRGBA8 c, std::uint32_t sort_key = 0) noexcept {
            Cmd cmd{};
            cmd.op = Op::VLine;
            cmd.color = c;
            cmd.sort_key = sort_key;
            cmd.x0 = x; cmd.y0 = y0; cmd.y1 = y1;
            return push_(cmd);
        }

    private:
        bool push_(const Cmd& c) noexcept {
            const std::uint32_t idx = m_count.fetch_add(1, std::memory_order_acq_rel);
            if (idx >= static_cast<std::uint32_t>(kMax)) {
                m_dropped.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
            m_cmds[idx] = c;
            return true;
        }

        std::array<Cmd, kMax> m_cmds{};
        std::atomic<std::uint32_t> m_count{0};
        std::atomic<std::uint32_t> m_dropped{0};
    };

    /**
     * @brief sort_key 정책 (z-index 대용)
     *
     *  [31:24] layer  (0..255)   : 큰 레이어
     *  [23:12] order  (0..4095)  : 레이어 내부 순서 (z-index 역할)
     *  [11: 0] tie    (0..4095)  : 동점 깨기(엔티티/좌표 기반 등)
     *
     * 작은 값이 먼저 실행(먼저 그려짐 = 뒤쪽),
     * 큰 값이 나중 실행(나중에 그려짐 = 앞쪽).
     */
    constexpr std::uint32_t make_sort_key(
        std::uint8_t layer,
        std::uint16_t order,
        std::uint16_t tie = 0
    ) noexcept {
        return (std::uint32_t(layer) << 24)
             | ((std::uint32_t(order) & 0x0FFFu) << 12)
             | (std::uint32_t(tie) & 0x0FFFu);
    }

    constexpr std::uint8_t  sort_layer(std::uint32_t k) noexcept { return std::uint8_t((k >> 24) & 0xFFu); }
    constexpr std::uint16_t sort_order(std::uint32_t k) noexcept { return std::uint16_t((k >> 12) & 0x0FFFu); }
    constexpr std::uint16_t sort_tie  (std::uint32_t k) noexcept { return std::uint16_t((k >>  0) & 0x0FFFu); }


} // namespace framedot::gfx
// include/framedot/gfx/RenderQueue.hpp
/**
 * @file RenderQueue.hpp
 * @brief RenderPrep 단계에서 무엇을 그릴지 기록하는 고정 용량 커맨드 큐.
 *
 * 설계 포인트:
 * - MPSC(멀티 프로듀서) push 안전
 * - 컨슈머(SoftwareRenderer)는 publish된 연속 구간만 읽는다.
 * - Text는 per-frame 고정 arena에 복사하여 수명 문제 제거
 * - Sprite는 외부 픽셀 포인터를 payload로 참조(수명은 유저가 보장)
 */
#pragma once
#include <framedot/gfx/Color.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string_view>


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
            BlendRect,
            FillCircle,
            Circle,
            BlitSprite,  // RGBA8888 블릿
            Text,        // debug text (ASCII/UTF-8 raw)
        };

        struct Cmd {
            Op op{};
            ColorRGBA8 color{0, 0, 0, 255};
            std::uint32_t sort_key{0};

            // 공통 좌표:
            // - rect: (x0,y0,w=x1,h=y1)
            // - line: (x0,y0)-(x1,y1)
            // - circle: center=(x0,y0), radius=x1
            //
            // - text: (x0,y0), x1=text_offset, y1=text_len
            // - sprite: (x0,y0,w=x1,h=y1), u0=stride_pixels
            std::int32_t x0{0}, y0{0};
            std::int32_t x1{0}, y1{0};

            // 옵션/플래그 확장
            std::uint16_t u0{0};
            std::uint16_t u1{0};
        };

        static constexpr std::size_t kMax = 8192;

        // text arena (프레임당 고정)
        static constexpr std::size_t kTextArenaBytes = 16 * 1024;

        RenderQueue() {
            // m_seq는 0으로 초기화되며, frame_id는 1부터 시작
            m_frame.store(1, std::memory_order_relaxed);
        }

        void begin_frame() noexcept {
            // frame_id 증가: 슬롯 clear 없이 이번 프레임에 준비되었는지를 seq 비교로 판별
            m_frame.fetch_add(1, std::memory_order_acq_rel);

            m_claimed.store(0, std::memory_order_release);
            m_published.store(0, std::memory_order_release);
            m_dropped.store(0, std::memory_order_release);
            m_text_ofs.store(0, std::memory_order_release);
        }

        std::size_t size() const noexcept {
            const std::uint32_t p = m_published.load(std::memory_order_acquire);
            return (p < (std::uint32_t)kMax) ? (std::size_t)p : kMax;
        }

        const Cmd* data() const noexcept { return m_cmds.data(); }

        std::uintptr_t payload0(std::size_t i) const noexcept { return m_p0[i]; }
        std::uintptr_t payload1(std::size_t i) const noexcept { return m_p1[i]; }

        std::uint32_t dropped() const noexcept {
            return m_dropped.load(std::memory_order_acquire);
        }

        const char* text_data(std::uint32_t ofs) const noexcept {
            if (ofs >= (std::uint32_t)kTextArenaBytes) return "";
            return &m_text[ofs];
        }

        // ----------------------------
        // API
        // ----------------------------

        bool clear(ColorRGBA8 c, std::uint32_t sort_key = 0) noexcept {
            Cmd cmd{};
            cmd.op = Op::Clear;
            cmd.color = c;
            cmd.sort_key = sort_key;
            return push_(cmd);
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

        // ---- Sprite ----
        // pixels: RGBA8888 (0xRRGGBBAA), stride_pixels >= w
        // color: tint(곱) + alpha(추가) 용도로 사용(일단 간단히: src*color)
        bool blit_sprite(std::int32_t x, std::int32_t y,
                         const std::uint32_t* pixels,
                         std::int32_t w, std::int32_t h,
                         std::uint16_t stride_pixels,
                         ColorRGBA8 tint,
                         std::uint32_t sort_key = 0) noexcept {
            if (!pixels || w <= 0 || h <= 0) return false;

            Cmd cmd{};
            cmd.op = Op::BlitSprite;
            cmd.color = tint;
            cmd.sort_key = sort_key;
            cmd.x0 = x; cmd.y0 = y;
            cmd.x1 = w; cmd.y1 = h;
            cmd.u0 = stride_pixels;
            return push_payload_(cmd, (std::uintptr_t)pixels, 0);
        }

        // ---- Text ----
        bool text(std::int32_t x, std::int32_t y,
                std::string_view utf8,
                ColorRGBA8 color,
                std::uint32_t sort_key,
                std::uint8_t scale) noexcept
        {
            if (utf8.empty()) return true;

            const std::uint32_t len = (std::uint32_t)utf8.size();
            const std::uint32_t ofs = m_text_ofs.fetch_add(len + 1, std::memory_order_acq_rel);
            if (ofs + len + 1 > (std::uint32_t)kTextArenaBytes) {
                m_dropped.fetch_add(1, std::memory_order_relaxed);
                return false;
            }

            std::memcpy(&m_text[ofs], utf8.data(), len);
            m_text[ofs + len] = '\0';

            Cmd cmd{};
            cmd.op = Op::Text;
            cmd.color = color;
            cmd.sort_key = sort_key;
            cmd.x0 = x; cmd.y0 = y;
            cmd.x1 = (std::int32_t)ofs;
            cmd.y1 = (std::int32_t)len;
            cmd.u0 = (std::uint16_t)((scale == 0) ? 1 : scale); // ★ scale 저장
            return push_(cmd);
        }

        bool text(
            std::int32_t x, std::int32_t y,
            std::string_view utf8,
            ColorRGBA8 color,
            std::uint32_t sort_key = 0
        ) noexcept {
            return text(x, y, utf8, color, sort_key, 1);
        }

    private:
        bool push_(const Cmd& c) noexcept {
            return push_payload_(c, 0, 0);
        }

        bool push_payload_(const Cmd& c, std::uintptr_t p0, std::uintptr_t p1) noexcept {
            const std::uint32_t frame = m_frame.load(std::memory_order_acquire);

            // 1) claim
            const std::uint32_t idx = m_claimed.fetch_add(1, std::memory_order_acq_rel);
            if (idx >= (std::uint32_t)kMax) {
                m_dropped.fetch_add(1, std::memory_order_relaxed);
                return false;
            }

            // 2) write
            m_cmds[idx] = c;
            m_p0[idx] = p0;
            m_p1[idx] = p1;

            // 3) mark-ready (release)
            m_seq[idx].store(frame, std::memory_order_release);

            // 4) publish contiguous frontier
            publish_(frame);
            return true;
        }

        void publish_(std::uint32_t frame) noexcept {
            while (true) {
                std::uint32_t p = m_published.load(std::memory_order_acquire);
                if (p >= (std::uint32_t)kMax) return;

                // 다음 슬롯이 이번 frame에서 준비됐는지 확인
                if (m_seq[p].load(std::memory_order_acquire) != frame) return;

                // frontier를 1칸 전진
                if (m_published.compare_exchange_weak(
                        p, p + 1,
                        std::memory_order_acq_rel,
                        std::memory_order_relaxed)) {
                    continue;
                }
            }
        }

        // storage
        std::array<Cmd, kMax> m_cmds{};
        std::array<std::uintptr_t, kMax> m_p0{};
        std::array<std::uintptr_t, kMax> m_p1{};

        // publish protocol
        std::atomic<std::uint32_t> m_frame{1};       // current frame id
        std::atomic<std::uint32_t> m_claimed{0};     // reserved slots
        std::atomic<std::uint32_t> m_published{0};   // contiguous published count
        std::atomic<std::uint32_t> m_dropped{0};
        std::array<std::atomic<std::uint32_t>, kMax> m_seq{}; // slot readiness marker

        // text arena
        std::array<char, kTextArenaBytes> m_text{};
        std::atomic<std::uint32_t> m_text_ofs{0};
    };

    // ---- sort_key helpers (z-index 대용) ----
    constexpr std::uint32_t make_sort_key(std::uint8_t layer,
                                         std::uint16_t order,
                                         std::uint16_t tie = 0) noexcept {
        return (std::uint32_t(layer) << 24)
             | ((std::uint32_t(order) & 0x0FFFu) << 12)
             | (std::uint32_t(tie) & 0x0FFFu);
    }

    constexpr std::uint8_t  sort_layer(std::uint32_t k) noexcept { return std::uint8_t((k >> 24) & 0xFFu); }
    constexpr std::uint16_t sort_order(std::uint32_t k) noexcept { return std::uint16_t((k >> 12) & 0x0FFFu); }
    constexpr std::uint16_t sort_tie  (std::uint32_t k) noexcept { return std::uint16_t((k >>  0) & 0x0FFFu); }

} // namespace framedot::gfx
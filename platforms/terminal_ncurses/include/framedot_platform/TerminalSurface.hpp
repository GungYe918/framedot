// platforms/terminal_ncurses/include/framedot_platform/TerminalSurface.hpp
/**
 * @file TerminalSurface.hpp
 * @brief ncurses 기반 출력 어댑터. PixelFrame(view)을 받아 터미널로 변환 출력
 */
#pragma once
#include <framedot/rhi/Surface.hpp>
#include <cstdint>
#include <vector>


namespace framedot::platform::terminal {

    class TerminalSurface final : public framedot::rhi::Surface {
    public:
        TerminalSurface();
        ~TerminalSurface() override;

        TerminalSurface(const TerminalSurface&) = delete;
        TerminalSurface& operator=(const TerminalSurface&) = delete;

        // Canvas -> terminal present
        void present(const framedot::gfx::PixelFrame& frame) override;

        // Non-blocking key poll. Returns -1 if no key.
        int poll_key() noexcept;

    private:
        void ensure_init_();
        void ensure_backbuffer_(int rows, int cols);
        void full_redraw_() noexcept;

        bool m_inited{false};
        bool m_color_ok{false};

        int m_rows{0};
        int m_cols{0};

        std::vector<std::uint32_t> m_prev;
    };

} // namespace framedot::platform::terminal
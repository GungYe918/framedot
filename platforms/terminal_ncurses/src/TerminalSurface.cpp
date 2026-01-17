// platforms/terminal_ncurses/src/TerminalSurface.cpp
#include <framedot_platform/TerminalSurface.hpp>
#include <framedot/gfx/PixelFrame.hpp>

#include <algorithm>
#include <cstdint>

#include <curses.h>


namespace framedot::platform::terminal {

    static short quantize_to_curses_bg(std::uint32_t rgba) {
        using framedot::gfx::PixelFrame;

        const int a = PixelFrame::a(rgba);
        if (a == 0) return COLOR_BLACK;

        const int r = PixelFrame::r(rgba);
        const int g = PixelFrame::g(rgba);
        const int b = PixelFrame::b(rgba);

        const int lum = (r * 30 + g * 59 + b * 11) / 100;

        const int maxc = std::max({r, g, b});
        const int minc = std::min({r, g, b});

        if (maxc < 40) return COLOR_BLACK;
        if (minc > 215) return COLOR_WHITE;
        if ((maxc - minc) < 25) return (lum > 128) ? COLOR_WHITE : COLOR_BLACK;

        const bool r_hi = r > 150;
        const bool g_hi = g > 150;
        const bool b_hi = b > 150;

        if (r_hi && g_hi && !b_hi) return COLOR_YELLOW;
        if (g_hi && b_hi && !r_hi) return COLOR_CYAN;
        if (r_hi && b_hi && !g_hi) return COLOR_MAGENTA;

        if (r >= g && r >= b) return COLOR_RED;
        if (g >= r && g >= b) return COLOR_GREEN;
        return COLOR_BLUE;
    }

    TerminalSurface::TerminalSurface() {
        ensure_init_();
    }

    TerminalSurface::~TerminalSurface() {
        if (m_inited) {
            // restore terminal
            endwin();
            m_inited = false;
        }
    }

    void TerminalSurface::ensure_init_() {
        if (m_inited) return;

        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        nodelay(stdscr, TRUE);   // non-blocking getch
        curs_set(0);             // hide cursor

        m_color_ok = false;
        if (has_colors()) {
            start_color();
            // allow terminal default bg if supported
            (void)use_default_colors();

            // pair id 1..8 used as bg colors (fg always black)
            // NOTE: COLOR_* constants are small ints; we map by explicit pairs.
            init_pair(1, COLOR_BLACK, COLOR_BLACK);
            init_pair(2, COLOR_BLACK, COLOR_RED);
            init_pair(3, COLOR_BLACK, COLOR_GREEN);
            init_pair(4, COLOR_BLACK, COLOR_YELLOW);
            init_pair(5, COLOR_BLACK, COLOR_BLUE);
            init_pair(6, COLOR_BLACK, COLOR_MAGENTA);
            init_pair(7, COLOR_BLACK, COLOR_CYAN);
            init_pair(8, COLOR_BLACK, COLOR_WHITE);

            m_color_ok = true;
        }

        // initial size
        int rows = 0, cols = 0;
        getmaxyx(stdscr, rows, cols);
        m_rows = rows;
        m_cols = cols;
        ensure_backbuffer_(rows, cols);

        // clear screen once
        erase();
        refresh();

        m_inited = true;
    }

    void TerminalSurface::ensure_backbuffer_(int rows, int cols) {
        if (rows <= 0 || cols <= 0) return;
        const std::size_t n = static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols);
        if (m_prev.size() != n) {
            m_prev.assign(n, 0xFFFFFFFFu); // force redraw on first present
        }
    }

    void TerminalSurface::full_redraw_() noexcept {
        // force next present to redraw all cells
        std::fill(m_prev.begin(), m_prev.end(), 0xFFFFFFFFu);
    }

    int TerminalSurface::poll_key() noexcept {
        ensure_init_();
        const int ch = getch();
        return (ch == ERR) ? -1 : ch;
    }

    void TerminalSurface::present(const framedot::gfx::PixelFrame& frame) {
        ensure_init_();

        int rows = 0, cols = 0;
        getmaxyx(stdscr, rows, cols);

        if (rows != m_rows || cols != m_cols) {
            m_rows = rows;
            m_cols = cols;
            ensure_backbuffer_(rows, cols);
            erase();
            full_redraw_();
        }

        if (!frame.valid()) {
            refresh();
            return;
        }

        const int cw = static_cast<int>(frame.width);
        const int ch = static_cast<int>(frame.height);

        const int draw_w = std::max(0, std::min(cols, cw));
        const int draw_h = std::max(0, std::min(rows, ch));

        const std::uint32_t* src = frame.pixels.data();
        const int stride = static_cast<int>(frame.stride_pixels);

        for (int y = 0; y < draw_h; ++y) {
            const int row_off_src = y * stride;
            const int row_off_dst = y * cols;

            for (int x = 0; x < draw_w; ++x) {
                const std::uint32_t p = src[row_off_src + x];

                const std::size_t di = static_cast<std::size_t>(row_off_dst + x);
                if (m_prev[di] == p) continue;
                m_prev[di] = p;

                if (m_color_ok) {
                    const short bg = quantize_to_curses_bg(p);
                    short pair_id = 1;
                    switch (bg) {
                        case COLOR_BLACK:   pair_id = 1; break;
                        case COLOR_RED:     pair_id = 2; break;
                        case COLOR_GREEN:   pair_id = 3; break;
                        case COLOR_YELLOW:  pair_id = 4; break;
                        case COLOR_BLUE:    pair_id = 5; break;
                        case COLOR_MAGENTA: pair_id = 6; break;
                        case COLOR_CYAN:    pair_id = 7; break;
                        case COLOR_WHITE:   pair_id = 8; break;
                        default:            pair_id = 1; break;
                    }
                    mvaddch(y, x, ' ' | COLOR_PAIR(pair_id));
                } else {
                    const int r = framedot::gfx::PixelFrame::r(p);
                    const int g = framedot::gfx::PixelFrame::g(p);
                    const int b = framedot::gfx::PixelFrame::b(p);
                    const int lum = (r * 30 + g * 59 + b * 11) / 100;

                    char c = ' ';
                    if (lum > 220) c = '@';
                    else if (lum > 180) c = '#';
                    else if (lum > 140) c = '*';
                    else if (lum > 100) c = '+';
                    else if (lum >  60) c = '.';
                    else c = ' ';

                    mvaddch(y, x, c);
                }
            }
        }

        refresh();
    }

} // namespace framedot::platform::terminal
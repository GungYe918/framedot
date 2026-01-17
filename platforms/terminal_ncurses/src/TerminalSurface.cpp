// platforms/terminal_ncurses/src/TerminalSurface.cpp
#include <framedot_platform/TerminalSurface.hpp>
#include <framedot/gfx/Color.hpp>
#include <framedot/gfx/PixelCanvas.hpp>

#include <algorithm>
#include <cstdint>

#include <curses.h>


namespace framedot::platform::terminal {

    static inline std::uint32_t u8(std::uint8_t v) { return static_cast<std::uint32_t>(v); }

    // Same packing rule as PixelCanvas (0xRRGGBBAA)
    static inline std::uint32_t pack_rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
        return (u8(r) << 24) | (u8(g) << 16) | (u8(b) << 8) | u8(a);
    }

    static constexpr std::uint8_t rr(std::uint32_t p) { return static_cast<std::uint8_t>((p >> 24) & 0xFF); }
    static constexpr std::uint8_t gg(std::uint32_t p) { return static_cast<std::uint8_t>((p >> 16) & 0xFF); }
    static constexpr std::uint8_t bb(std::uint32_t p) { return static_cast<std::uint8_t>((p >>  8) & 0xFF); }
    static constexpr std::uint8_t aa(std::uint32_t p) { return static_cast<std::uint8_t>((p >>  0) & 0xFF); }

    static short quantize_to_curses_bg(std::uint32_t rgba) {
        const int a = aa(rgba);
        if (a == 0) return COLOR_BLACK;

        const int r = rr(rgba);
        const int g = gg(rgba);
        const int b = bb(rgba);

        // simple luminance-ish
        const int lum = (r * 30 + g * 59 + b * 11) / 100;

        // very dark -> black, very bright & neutral -> white
        const int maxc = std::max({r, g, b});
        const int minc = std::min({r, g, b});

        if (maxc < 40) return COLOR_BLACK;
        if (minc > 215) return COLOR_WHITE;

        // near-gray (low saturation): choose white/black by luminance
        if ((maxc - minc) < 25) return (lum > 128) ? COLOR_WHITE : COLOR_BLACK;

        // detect 2-channel mixes first (yellow/cyan/magenta)
        const bool r_hi = r > 150;
        const bool g_hi = g > 150;
        const bool b_hi = b > 150;

        if (r_hi && g_hi && !b_hi) return COLOR_YELLOW;
        if (g_hi && b_hi && !r_hi) return COLOR_CYAN;
        if (r_hi && b_hi && !g_hi) return COLOR_MAGENTA;

        // dominant single channel
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

    void TerminalSurface::present(const framedot::gfx::PixelCanvas& canvas) {
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

        const int cw = static_cast<int>(canvas.width());
        const int ch = static_cast<int>(canvas.height());

        const int draw_w = std::max(0, std::min(cols, cw));
        const int draw_h = std::max(0, std::min(rows, ch));

        auto px = canvas.pixels(); // span<const Pixel>
        const std::uint32_t* src = reinterpret_cast<const std::uint32_t*>(px.data());

        // Draw only visible region
        for (int y = 0; y < draw_h; ++y) {
            const int row_off_src = y * cw;
            const int row_off_dst = y * cols;

            for (int x = 0; x < draw_w; ++x) {
                const std::uint32_t p = src[row_off_src + x];

                const std::size_t di = static_cast<std::size_t>(row_off_dst + x);
                if (m_prev[di] == p) continue;
                m_prev[di] = p;

                if (m_color_ok) {
                    const short bg = quantize_to_curses_bg(p);
                    // map bg color to pair id
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

                    // paint background by printing a space with bg color
                    mvaddch(y, x, ' ' | COLOR_PAIR(pair_id));
                } else {
                    // no color: approximate using ASCII density by luminance
                    const int r = rr(p), g = gg(p), b = bb(p);
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

        // If terminal larger than canvas, you can optionally clear the rest.
        // (We keep it as-is for now to avoid extra work.)

        refresh();

    }

} // namespace framedot::platform::terminal
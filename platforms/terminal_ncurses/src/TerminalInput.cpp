// platforms/terminal_ncurses/src/TerminalInput.cpp
#include <framedot_platform/TerminalInput.hpp>
#include <framedot/input/Event.hpp>
#include <framedot/input/Key.hpp>

#include <curses.h>


namespace framedot::platform::terminal {

    static framedot::input::Key map_key_(int ch) {
        using framedot::input::Key;

        /// @brief ncurses 키코드를 엔진 키로 변환
        switch(ch) {
            case KEY_LEFT:  return Key::Left;
            case KEY_RIGHT: return Key::Right;
            case KEY_UP:    return Key::Up;
            case KEY_DOWN:  return Key::Down;

            case 27:        return Key::Escape; // ESC
            case 10:        return Key::Enter;  // LF
            case KEY_ENTER: return Key::Enter;
            case ' ':       return Key::Space;

            case 'q': case 'Q': return Key::Q;
            case 'w': case 'W': return Key::W;
            case 'a': case 'A': return Key::A;
            case 's': case 'S': return Key::S;
            case 'd': case 'D': return Key::D;

            default: return Key::Unknown;
        }

        return Key::Unknown;
    }

    /// @brief 가능한 입력을 다 읽어서 out에 밀어넣는다.
    /// - 터미널은 key release를 얻기 어렵기 때문에 Press 위주
    void TerminalInput::pump(framedot::input::InputQueue& out) {
        using namespace framedot::input;

        while (true) {
            const int ch = m_surf.poll_key();
            if (ch < 0) break;

            const Key k = map_key_(ch);
            if (k == Key::Unknown) {
                /// @brief 아직 매핑하지 않은 키는 무시
                continue;
            }

            Event ev{};
            ev.type = EventType::Key;
            ev.data.key = KeyEvent{ k, KeyAction::Press };

            (void)out.push(ev); // 큐가 가득 차면 드랍
        }
    }

} // namespace framedot::platform::terminal
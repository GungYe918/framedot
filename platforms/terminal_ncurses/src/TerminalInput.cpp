// platforms/terminal_ncurses/src/TerminalInput.cpp
#include <framedot_platform/TerminalInput.hpp>
#include <framedot/input/Event.hpp>
#include <framedot/input/Key.hpp>

#include <curses.h>


namespace framedot::platform::terminal {

    static framedot::input::Key map_key_(int ch) {
        using framedot::input::Key;

        /// @brief ncurses 키코드를 엔진 키로 변환
        switch (ch) {
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
    }

    void TerminalInput::pump(framedot::input::InputCollector& collector) {
        using namespace framedot::input;

        /// @brief 가능한 입력을 다 읽어서 collector에 밀어넣는다.
        /// - 터미널은 key release를 얻기 어렵기 때문에 Press 위주.
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

            /// @brief 오버플로 시에도 상태는 유지되고, 이벤트 기록만 정책에 따라 드랍될 수 있다.
            (void)collector.push(ev);
        }
    }

} // namespace framedot::platform::terminal

// platforms/terminal_ncurses/include/framedot_platform/TerminalInput.hpp
#pragma once
#include <framedot/input/InputSource.hpp>
#include <framedot_platform/TerminalSurface.hpp>

namespace framedot::platform::terminal {

    class TerminalInput final : public framedot::input::InputSource {
    public:
        /// @brief TerminalSurface가 가진 ncurses 컨텍스트(getch 설정 등)를 재사용
        explicit TerminalInput(TerminalSurface& surf) : m_surf(surf) {}

        /// @brief 논블로킹으로 입력을 가능한 만큼 읽어서 collector에 push
        void pump(framedot::input::InputCollector& collector) override;

    private:
        TerminalSurface& m_surf;
    };

} // namespace framedot::platform::terminal
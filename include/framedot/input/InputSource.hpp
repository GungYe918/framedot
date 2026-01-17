// framedot/input/InputSource.hpp
#pragma once
#include <framedot/input/InputCollector.hpp>


namespace framedot::input {

    /// @brief 플랫폼/디바이스에서 입력을 읽어 InputCollector에 주입하는 인터페이스
    /// - ncurses, SDL, 브라우저 이벤트, 콘솔 등
    class InputSource {
    public:
        virtual ~InputSource() = default;

        /// @brief 가능한 입력을 모두 폴링하여 collector로 적재
        virtual void pump(InputCollector& collector) = 0;
    };

} // namespace framedot::input
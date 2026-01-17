// framedot/input/InputSource.hpp
#pragma once
#include <framedot/input/InputQueue.hpp>


namespace framedot::input {

    /// @brief 플랫폼/디바이스에서 입력을 읽어 InputQueue에 주입하는 인터페이스.
    /// - ncurses, SDL, WebGPU/Browser event, 콘솔 등.
    class InputSource {
    public:
        virtual ~InputSource() = default;

        /// @brief 가능한 입력을 모두 폴링하여 큐에 적재
        virtual void pump(InputQueue& out) = 0;
    };

} // namespace framedot::input
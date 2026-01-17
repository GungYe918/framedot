// include/framedot/input/Key.hpp
#pragma once
#include <cstdint>


namespace framedot::input {

    /// @brief 키 식별자
    enum class Key : std::uint16_t {
        Unknown = 0,

        Left, Right, Up, Down,
        Space, Enter, Escape,

        // 알파벳 (우선 최소만)
        Q, W, A, S, D,

        /// @brief 키 개수 (배열 크기 산정용)
        Count
    };


    /// @brief 키 액션. 터미널은 release를 얻기 어렵기 때문에 당장은 Press 중심.
    enum class KeyAction : std::uint8_t {
        Press = 0,
        Release,
        Repeat,
    };

} // namespace framedot::input
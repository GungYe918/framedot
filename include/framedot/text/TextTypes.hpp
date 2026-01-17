// include/framedot/text/TextTypes.hpp
#pragma once
#include <cstdint>


namespace framedot::text {

    enum class Direction : std::uint8_t {
        LTR,
        RTL
    };

    struct FontHandle {
        std::uint32_t id{0}; // 0 = invalid
    };

} // namespace framedot::text
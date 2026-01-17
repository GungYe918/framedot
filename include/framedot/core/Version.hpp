// include/framedot/Version.hpp
#pragma once
#include <cstdint>


namespace framedot::core {

    struct Version {
        std::uint32_t major;
        std::uint32_t minor;
        std::uint32_t patch;
    };

    Version version() noexcept;
    const char* version_string() noexcept;

} // namespace framedot::core
// src/core/version.cpp
#include <framedot/core/Version.hpp>


namespace framedot::core {

    Version version() noexcept {
        return Version{
            static_cast<std::uint32_t>(FRAMEDOT_VERSION_MAJOR),
            static_cast<std::uint32_t>(FRAMEDOT_VERSION_MINOR),
            static_cast<std::uint32_t>(FRAMEDOT_VERSION_PATCH),
        };
    }

    const char* version_string() noexcept {
        // 간단 버전: 필요하면 나중에 문자열 캐싱/생성으로 개선
        return "framedot " /* + ... */ ;
    }

} // namespace framedot::core

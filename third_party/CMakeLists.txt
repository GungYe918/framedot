# third_party/CMakeLists.txt
include(FetchContent)

if(FRAMEDOT_USE_SYSTEM_DEPS)
  find_package(fmt REQUIRED)
else()
  # fmt
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        10.2.1
  )
  FetchContent_MakeAvailable(fmt)
endif()

# EnTT
FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG        v3.13.2
)
FetchContent_MakeAvailable(entt)
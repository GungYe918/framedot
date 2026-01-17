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

# GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.3
)
FetchContent_MakeAvailable(glm)

# Harfbuzz
if(FRAMEDOT_USE_SYSTEM_DEPS)
  find_package(harfbuzz REQUIRED)
else()
  FetchContent_Declare(
    harfbuzz
    GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
    GIT_TAG        8.4.0
  )
  FetchContent_MakeAvailable(harfbuzz)
endif()

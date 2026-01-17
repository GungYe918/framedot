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

# --------------------------
# HarfBuzz (normalize)
# --------------------------
if(FRAMEDOT_USE_SYSTEM_DEPS)

  # Try multiple package names quietly (distros differ)
  # Common: HarfBuzz (CMake package name)
  # Sometimes: harfbuzz (lowercase), harfbuzz-ng, etc.
  set(_framedot_hb_found OFF)

  foreach(_pkg IN ITEMS HarfBuzz harfbuzz Harfbuzz)
    if(NOT _framedot_hb_found)
      find_package(${_pkg} QUIET)
      if(TARGET HarfBuzz::harfbuzz OR TARGET harfbuzz::harfbuzz OR TARGET harfbuzz)
        set(_framedot_hb_found ON)
      endif()
    endif()
  endforeach()

  if(NOT _framedot_hb_found)
    message(FATAL_ERROR "HarfBuzz package not found via find_package(). Tried: HarfBuzz, harfbuzz, Harfbuzz. "
                        "Install system HarfBuzz (dev) package or set FRAMEDOT_USE_SYSTEM_DEPS=OFF.")
  endif()

else()
  FetchContent_Declare(
    harfbuzz
    GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
    GIT_TAG        8.4.0
  )
  FetchContent_MakeAvailable(harfbuzz)
endif()

# ---- normalize target name to framedot::harfbuzz ----
if(TARGET framedot::harfbuzz)
  # already defined elsewhere; do nothing
elseif(TARGET harfbuzz::harfbuzz)
  add_library(framedot::harfbuzz ALIAS harfbuzz::harfbuzz)
elseif(TARGET HarfBuzz::harfbuzz)
  add_library(framedot::harfbuzz ALIAS HarfBuzz::harfbuzz)
elseif(TARGET harfbuzz)
  add_library(framedot::harfbuzz ALIAS harfbuzz)
else()
  message(FATAL_ERROR "HarfBuzz target not found after acquisition. Known candidates: "
                      "harfbuzz::harfbuzz, HarfBuzz::harfbuzz, harfbuzz")
endif()

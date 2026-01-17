# root/cmake
cmake_minimum_required(VERSION 3.20)

project(framedot
    VERSION 0.1.0
    LANGUAGES CXX
)

# ---------------------------------------
# Options
# ---------------------------------------
option(FRAMEDOT_BUILD_TESTS     "Build framedot tests" ON)
option(FRAMEDOT_BUILD_EXAMPLES  "Build framedot examples" ON)
option(FRAMEDOT_BUILD_PLATFORMS "Build platform adapters (ncurses etc.)" OFF)
option(FRAMEDOT_USE_SYSTEM_DEPS "Use system packages instead of vendored third_party" OFF)

# ---------------------------------------
# Global CMake settings
# ---------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(framedotOptions)
include(framedotWarnings)
include(framedotInstall)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(NOT FRAMEDOT_INPUT_OVERFLOW_POLICY MATCHES "^[0-2]$")
  message(FATAL_ERROR "FRAMEDOT_INPUT_OVERFLOW_POLICY must be 0, 1, or 2. Got: ${FRAMEDOT_INPUT_OVERFLOW_POLICY}")
endif()

# --- Generate config header (single source of truth) ---
set(FRAMEDOT_GENERATED_DIR "${PROJECT_BINARY_DIR}/generated")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/framedotConfig.hpp.in"
  "${FRAMEDOT_GENERATED_DIR}/framedot/core/Config.hpp"
  @ONLY
)


# ---------------------------------------
# Dependencies (placeholder)
# ---------------------------------------
add_subdirectory(third_party)

# ---------------------------------------
# Engine library
# ---------------------------------------
add_subdirectory(src)

# ---------------------------------------
# Examples / Tests
# ---------------------------------------
if(FRAMEDOT_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(FRAMEDOT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(FRAMEDOT_BUILD_PLATFORMS)
  add_subdirectory(platforms)
endif()

# ---------------------------------------
# Install / Package config
# ---------------------------------------
framedot_setup_install()

# src/CMakeLists.txt

add_library(framedot
  core/version.cpp
  core/job_system.cpp
  app/run_loop.cpp
  ecs/world.cpp
  gfx/pixel_canvas.cpp
  gfx/software_renderer.cpp
)

add_library(framedot::framedot ALIAS framedot)

target_include_directories(framedot
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${PROJECT_SOURCE_DIR}/internal
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(framedot
  PUBLIC
    fmt::fmt
    EnTT::EnTT
)

target_compile_definitions(framedot PUBLIC
  FRAMEDOT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  FRAMEDOT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  FRAMEDOT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Warnings
framedot_apply_warnings(framedot)

# Install library
install(TARGETS framedot
  EXPORT framedotTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
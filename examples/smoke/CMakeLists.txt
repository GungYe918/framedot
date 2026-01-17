# examples/smoke/cmake
add_executable(framedot_smoke
  src/main.cpp
)
target_link_libraries(framedot_smoke PRIVATE framedot::framedot)
find_package(Curses REQUIRED)

add_executable(framedot_term_probe
  src/main.cpp
  src/TerminalSurface.cpp
  src/TerminalInput.cpp
)

target_include_directories(framedot_term_probe PRIVATE
  ${PROJECT_SOURCE_DIR}/platforms/terminal_ncurses/include
)

target_link_libraries(framedot_term_probe PRIVATE
  framedot::framedot
  ${CURSES_LIBRARIES}
)

target_include_directories(framedot_term_probe PRIVATE
  ${CURSES_INCLUDE_DIRS}
)
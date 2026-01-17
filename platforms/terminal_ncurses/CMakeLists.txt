# platforms/terminal_ncurses/cmake
find_package(Curses REQUIRED)

add_library(framedot_terminal_ncurses
  src/TerminalSurface.cpp
  src/TerminalInput.cpp
)

add_library(framedot::terminal_ncurses ALIAS framedot_terminal_ncurses)

target_include_directories(framedot_terminal_ncurses
  PUBLIC
    ${PROJECT_SOURCE_DIR}/platforms/terminal_ncurses/include
  PRIVATE
    ${CURSES_INCLUDE_DIRS}
)

target_link_libraries(framedot_terminal_ncurses
  PUBLIC
    framedot::framedot
    ${CURSES_LIBRARIES}
)

# --------------------------
# Manual-run demo executable
# --------------------------
add_executable(framedot_term_ncurses_demo
  examples/term_demo.cpp
)

target_link_libraries(framedot_term_ncurses_demo PRIVATE
  framedot::terminal_ncurses
)

add_executable(framedot_tests test_sanity.cpp)
target_link_libraries(framedot_tests PRIVATE framedot::framedot)
add_test(NAME framedot_tests COMMAND framedot_tests)
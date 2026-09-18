add_executable(arda_tests
    src/main.cpp
    src/EllipsoidTests.cpp
    src/renderer/DeviceTests.cpp
)
target_link_libraries(arda_tests PRIVATE arda_core arda_renderer doctest::doctest)

add_test(NAME arda_tests COMMAND arda_tests)

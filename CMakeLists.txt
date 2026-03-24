cmake_minimum_required(VERSION 3.25)
project(LiveLy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(src)

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

add_executable(lively ${SOURCES})

include(CTest)

# Smoke-test lexer output against the hello example.
add_test(
    NAME lexer_hello
    COMMAND lively ${CMAKE_CURRENT_SOURCE_DIR}/examples/hello.lv
)

set_tests_properties(lexer_hello PROPERTIES
    PASS_REGULAR_EXPRESSION "BIND \(bind\);TYPE_INT \(int\);ASSIGN \(is\);EMIT \(emit\);EOF \(\)"
)

# Convenience target to run the sample manually from the build.
add_custom_target(run_hello
    COMMAND lively ${CMAKE_CURRENT_SOURCE_DIR}/examples/hello.lv
    DEPENDS lively
    COMMENT "Running lively on examples/hello.lv"
)

# Unified test entry point. As new tests are added with add_test(), this target
# continues to run all of them with one command.
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS lively
    COMMENT "Running all CTest tests"
)
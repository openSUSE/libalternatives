cmake_minimum_required(VERSION 3.0)
project(libalternatives)

set(SOURCES
    src/libalternatives.c
    src/parser.c
    src/config_parser.c
)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(alternatives SHARED ${SOURCES})
set_property(TARGET alternatives PROPERTY
    ALTERNATIVES_PATH test)
target_compile_definitions(alternatives PRIVATE
    ALTERNATIVES_PATH="TEST"
    CONFIG_DIR="TEST"
)

add_library(test_alternatives STATIC ${SOURCES})
target_compile_definitions(test_alternatives PRIVATE
    ALTERNATIVES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test/test_config"
    CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/test_defaults"
)


# disable the <libname>_EXPORTS
set_target_properties(alternatives PROPERTIES DEFINE_SYMBOL "")


add_executable(units test/test.c test/parser_tests.c test/config_parser_tests.c )
target_link_libraries(units PUBLIC cunit)
target_link_libraries(units PUBLIC test_alternatives)
set_property(TARGET test_alternatives alternatives PROPERTY C_STANDARD 99)

include(./cmake/CodeCoverage.cmake)
APPEND_COVERAGE_COMPILER_FLAGS()
setup_target_for_coverage_lcov(
    NAME coverage
    EXECUTABLE units
    EXCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/test/*
)


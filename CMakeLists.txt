cmake_minimum_required(VERSION 3.8)
project(libalternatives C)

set(SOURCES
    src/libalternatives.c
    src/options_parser.c
    src/config_parser.c
)

set(HEADERS
    src/libalternatives.h
)

add_compile_options(-Wall -Wextra -Wpedantic)

set_property(GLOBAL PROPERTY VERSION 0.0.1)

add_library(alternatives SHARED ${SOURCES})
set_property(TARGET alternatives PROPERTY SOVERSION 1)
set_property(TARGET alternatives PROPERTY
    ETC_PATH test)
target_compile_definitions(alternatives PUBLIC
    ETC_PATH="/etc"
    CONFIG_DIR="/usr/share/libalternatives"
    CONFIG_FILENAME="libalternatives.conf"
)

# test library also contains utilities that will be tested
add_library(test_alternatives STATIC
    ${SOURCES}
    src/utils/alternatives.c
)
target_compile_definitions(test_alternatives PRIVATE
    ETC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test"
    CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/test_defaults"
    CONFIG_FILENAME="libalternatives.conf"
    UNITTESTS=1
)


# disable the <libname>_EXPORTS
set_target_properties(alternatives PROPERTIES DEFINE_SYMBOL "")


add_executable(units
    test/test.c
    test/options_parser_tests.c
    test/config_parser_tests.c
    test/alternatives_tests.c
)
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


# utilities
add_executable(libalternatives_default
    src/utils/default_helper.c
)
target_link_libraries(libalternatives_default PUBLIC alternatives)
set_property(TARGET libalternatives_default PROPERTY SKIP_BUILD_RPATH TRUE)

add_executable(update_alternatives
    src/utils/alternatives.c
)
target_link_libraries(update_alternatives PUBLIC alternatives)
set_property(TARGET update_alternatives PROPERTY SKIP_BUILD_RPATH TRUE)
set_target_properties(update_alternatives PROPERTIES OUTPUT_NAME alternatives)


install(TARGETS libalternatives_default DESTINATION bin)
install(TARGETS update_alternatives     DESTINATION bin)
install(TARGETS alternatives            DESTINATION lib)
install(FILES ${HEADERS}                DESTINATION include)

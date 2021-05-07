if(BUILD_TESTING)
add_executable(units
    test.c
    options_parser_tests.c
    config_parser_tests.c
    alternatives_tests.c
)
target_link_libraries(units PUBLIC cunit)
target_link_libraries(units PUBLIC test_libalternatives)
target_compile_definitions(units PUBLIC CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_defaults")

add_test(NAME unit COMMAND units)

endif()

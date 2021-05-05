if(BUILD_TESTING)
add_executable(units
    test.c
    options_parser_tests.c
    config_parser_tests.c
    alternatives_tests.c
)
target_link_libraries(units PUBLIC cunit)
target_link_libraries(units PUBLIC test_libalternatives)

add_test(NAME unit COMMAND units)

endif()

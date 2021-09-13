set(test_SOURCES
    alternatives_tests.c
    config_parser_tests.c
    options_parser_tests.c
    test.c
)

if(BUILD_TESTING)
    add_executable(units ${test_SOURCES})
    target_link_libraries(units PRIVATE cunit)
    target_link_libraries(units PRIVATE TestAlternativeHelper TestLibalternatives)
    target_compile_definitions(units PUBLIC CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_defaults")

    add_executable(argv_replaced_helper argv_replaced_helper.c)

    add_test(NAME unit COMMAND units)
endif()

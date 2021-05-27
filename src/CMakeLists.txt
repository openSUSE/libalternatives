set(libalternative_SOURCES
    libalternative.c
    options_parser.c
    config_parser.c
)

set(libalternative_PUBLIC_HEADERS
    libalternative.h
)
set(libalternative_HEADERS
    ${libalternative_PUBLIC_HEADERS}
    parser.h
)

add_library(alternative SHARED "${libalternative_SOURCES}")
set_property(TARGET alternative PROPERTY SOVERSION 1)
set_property(TARGET alternative PROPERTY
    ETC_PATH test)
target_compile_definitions(alternative PUBLIC
    ETC_PATH="${CMAKE_INSTALL_SYSCONFDIR}"
    CONFIG_DIR="${CONFIG_DIR}"
    CONFIG_FILENAME="${CONFIG_FILENAME}"
)

# disable the <libname>_EXPORTS
set_target_properties(alternative PROPERTIES DEFINE_SYMBOL "")
set_property(TARGET alternative PROPERTY C_STANDARD 99)

if(BUILD_TESTING)
    # test library also contains utilities that will be tested
    add_library(TestLibalternative STATIC ${libalternative_SOURCES})
    target_compile_definitions(TestLibalternative PRIVATE
        ETC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../test"
        CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../test/test_defaults"
        CONFIG_FILENAME="${CONFIG_FILENAME}"
        UNITTESTS=1
    )

    set_property(TARGET TestLibalternative PROPERTY C_STANDARD 99)
endif()

install(FILES ${libalternative_PUBLIC_HEADERS} DESTINATION include)

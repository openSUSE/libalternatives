set(libalternatives_SOURCES
    libalternatives.c
    options_parser.c
    config_parser.c
)

set(libalternatives_PUBLIC_HEADERS
    libalternatives.h
)
set(libalternatives_HEADERS
    ${libalternatives_PUBLIC_HEADERS}
    parser.h
)

add_library(alternatives SHARED "${libalternatives_SOURCES}")
set_property(TARGET alternatives PROPERTY SOVERSION 1)
set_property(TARGET alternatives PROPERTY ETC_PATH test)
target_compile_definitions(alternatives PUBLIC
    ETC_PATH="/${CMAKE_INSTALL_SYSCONFDIR}"
    CONFIG_DIR="${CONFIG_DIR}"
    CONFIG_FILENAME="${CONFIG_FILENAME}"
)
set_target_properties(alternatives PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties(alternatives PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libalternatives.version)
set (CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libalternatives.version")

# disable the <libname>_EXPORTS
#set_target_properties(alternatives PROPERTIES DEFINE_SYMBOL "")
set_property(TARGET alternatives PROPERTY C_STANDARD 99)

if(BUILD_TESTING)
    # test library also contains utilities that will be tested
    add_library(TestLibalternatives STATIC ${libalternatives_SOURCES})
    target_compile_definitions(TestLibalternatives PRIVATE
        ETC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../test"
        CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../test/test_defaults"
        CONFIG_FILENAME="${CONFIG_FILENAME}"
        UNITTESTS=1
    )

    set_property(TARGET TestLibalternatives PROPERTY C_STANDARD 99)
endif()

install(FILES ${libalternatives_PUBLIC_HEADERS} DESTINATION include)
install(TARGETS alternatives)


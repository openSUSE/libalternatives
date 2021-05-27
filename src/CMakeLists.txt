set(SOURCES
    libalternative.c
    options_parser.c
    config_parser.c
)

set(HEADERS
    libalternative.h
)

add_library(alternative SHARED "${SOURCES}")
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

# utilities
add_executable(AlternativesHelper
    utils/alternative.c
    utils/list_binaries.c
    utils/group_consistency_rules.c
)
target_link_libraries(AlternativesHelper PUBLIC alternative)
set_property(TARGET AlternativesHelper PROPERTY SKIP_BUILD_RPATH TRUE)
set_target_properties(AlternativesHelper PROPERTIES OUTPUT_NAME alts)

if(BUILD_TESTING)
# test library also contains utilities that will be tested
add_library(test_libalternative STATIC
    "${SOURCES}"
    utils/alternative.c
    utils/list_binaries.c
    utils/group_consistency_rules.c
)
target_compile_definitions(test_libalternative PRIVATE
    ETC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../test"
    CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../test/test_defaults"
    CONFIG_FILENAME="${CONFIG_FILENAME}"
    UNITTESTS=1
)

set_property(TARGET test_libalternative PROPERTY C_STANDARD 99)
endif()

install(TARGETS alternative AlternativesHelper)
install(FILES ${HEADERS} DESTINATION include)

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

add_library(alternatives SHARED "${libalternatives_SOURCES}" "${libalternatives_HEADERS}")

set_target_properties(alternatives PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  C_STANDARD 99
  C_STANDARD_REQUIRED ON
  C_VISIBILITY_PRESET hidden
  PUBLIC_HEADER "${libalternatives_PUBLIC_HEADERS}"
  LINK_DEPENDS "${PROJECT_SOURCE_DIR}/src/libalternatives.version"
  LINK_FLAGS "-Wl,--version-script,\"${PROJECT_SOURCE_DIR}/src/libalternatives.version\""
)

set_property(TARGET alternatives PROPERTY ETC_PATH test)
target_compile_definitions(alternatives PUBLIC
    ETC_PATH="/${CMAKE_INSTALL_SYSCONFDIR}"
    CONFIG_DIR="${CONFIG_DIR}"
    CONFIG_FILENAME="${CONFIG_FILENAME}"
)

# Install the library
configure_file(${PROJECT_SOURCE_DIR}/cmake/libalternatives.pc.in ${CMAKE_BINARY_DIR}/libalternatives.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libalternatives.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
install(TARGETS alternatives
  EXPORT libalternativesTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Generate and install the exports
export(EXPORT libalternativesTargets
  FILE ${CMAKE_BINARY_DIR}/libalternativesTargets.cmake
)
install(EXPORT libalternativesTargets
  FILE libalternativesTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libalternatives
)
configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/libalternativesConfig.cmake.in
  ${CMAKE_BINARY_DIR}/libalternativesConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libalternatives
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
install(FILES
  ${CMAKE_BINARY_DIR}/libalternativesConfig.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libalternatives
)
write_basic_package_version_file(
  ${CMAKE_BINARY_DIR}/libalternativesConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMinorVersion
)
install(FILES
  ${CMAKE_BINARY_DIR}/libalternativesConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libalternatives
)


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


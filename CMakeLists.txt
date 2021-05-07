cmake_minimum_required(VERSION 3.8)
project(libalternatives
    VERSION 1.0.0
    LANGUAGES C
)

include(GNUInstallDirs)
include(CTest)

option(ENABLE_COVERAGE "Add coverage target" OFF)

set(CONFIG_DIR
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}"
    CACHE PATH
    "Root path for alternatives configs"
)
set(CONFIG_FILENAME "libalternatives.conf" CACHE STRING "Configueration filename in the SYSCONFDIR")
add_compile_options(-Wall -Wextra -Wpedantic)

add_subdirectory(src)
add_subdirectory(test)

if(ENABLE_COVERAGE)
    include(./cmake/CodeCoverage.cmake)
    APPEND_COVERAGE_COMPILER_FLAGS()
    setup_target_for_coverage_lcov(
        NAME coverage
        EXECUTABLE units
        EXCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/test/*
    )
endif()

# utilities
add_executable(libalternatives_default
    src/utils/default_helper.c
)
target_link_libraries(libalternatives_default PUBLIC alternatives)
set_property(TARGET libalternatives_default PROPERTY SKIP_BUILD_RPATH TRUE)

add_executable(AlternativesHelper
    src/utils/alternatives.c
)
target_link_libraries(AlternativesHelper PUBLIC alternatives)
set_property(TARGET AlternativesHelper PROPERTY SKIP_BUILD_RPATH TRUE)
set_target_properties(AlternativesHelper PROPERTIES OUTPUT_NAME alts)

#install(TARGETS libalternatives_default DESTINATION bin)
install(TARGETS AlternativesHelper)

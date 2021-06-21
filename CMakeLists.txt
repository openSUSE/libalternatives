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
    "Root path for alternative configs"
)
set(CONFIG_FILENAME "libalternatives.conf" CACHE STRING "Configueration filename in the SYSCONFDIR")
add_compile_options(-Wall -Wextra -Wpedantic)

if(ENABLE_COVERAGE)
    include(./cmake/CodeCoverage.cmake)
    APPEND_COVERAGE_COMPILER_FLAGS()
    setup_target_for_coverage_lcov(
        NAME coverage
        EXECUTABLE units
        EXCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/test/*
    )
endif()

add_subdirectory(src)
add_subdirectory(utils)
add_subdirectory(test)
add_subdirectory(doc)

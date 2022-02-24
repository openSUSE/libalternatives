cmake_minimum_required(VERSION 3.12)

project(libalternatives VERSION 1.2
        DESCRIPTION "A configurable alternative for update-alternatives."
        LANGUAGES C
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
find_package(PkgConfig REQUIRED)


option(ENABLE_COVERAGE "Add coverage target" OFF)

set(CONFIG_DIR
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}"
    CACHE PATH
    "Root path for alternative configs"
)
set(CONFIG_FILENAME "libalternatives.conf" CACHE STRING "Configueration filename in the SYSCONFDIR")
add_compile_options(-Wall -Wextra -Wpedantic -fvisibility=hidden)

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

add_test(check_version sh ${CMAKE_CURRENT_SOURCE_DIR}/release_tag.sh -c)

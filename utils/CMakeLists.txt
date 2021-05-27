set(alts_SOURCES
	alternative.c
	group_consistency_rules.c
	list_binaries.c
)

set(alts_HEADERS
	utils.h
)

add_executable(AlternativesHelper
	${alts_SOURCES}
)
target_link_libraries(AlternativesHelper PUBLIC alternative)
set_property(TARGET AlternativesHelper PROPERTY SKIP_BUILD_RPATH TRUE)
set_target_properties(AlternativesHelper PROPERTIES OUTPUT_NAME alts)

if(BUILD_TESTING)
	add_library(TestAlternativeHelper STATIC ${alts_SOURCES})
	target_compile_definitions(TestAlternativeHelper PRIVATE
        ETC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../test"
        CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../test/test_defaults"
        CONFIG_FILENAME="${CONFIG_FILENAME}"
        UNITTESTS=1
    )
    set_property(TARGET TestAlternativeHelper PROPERTY C_STANDARD 99)
endif()

install(TARGETS alternative AlternativesHelper)

include_guard (DIRECTORY)

set (PCRE_INCLUDE_DIR ${CCL_SUBMODULES_DIR}/pcre2/src CACHE PATH "")

configure_file(${PCRE_INCLUDE_DIR}/config.h.generic
               ${CMAKE_CURRENT_BINARY_DIR}/pcre2/include/config.h)

configure_file(${PCRE_INCLUDE_DIR}/pcre2.h.generic
               ${CMAKE_CURRENT_BINARY_DIR}/pcre2/include/pcre2.h)

configure_file(${PCRE_INCLUDE_DIR}/pcre2_chartables.c.dist
               ${CMAKE_CURRENT_BINARY_DIR}/pcre2/src/pcre2_chartables.c)

list (APPEND pcre_sources
    ${PCRE_INCLUDE_DIR}/pcre2_auto_possess.c
	${PCRE_INCLUDE_DIR}/pcre2_chkdint.c
	${PCRE_INCLUDE_DIR}/pcre2_compile.c
	${PCRE_INCLUDE_DIR}/pcre2_compile_cgroup.c
	${PCRE_INCLUDE_DIR}/pcre2_compile_class.c
	${PCRE_INCLUDE_DIR}/pcre2_context.c
	${PCRE_INCLUDE_DIR}/pcre2_extuni.c
	${PCRE_INCLUDE_DIR}/pcre2_find_bracket.c
	${PCRE_INCLUDE_DIR}/pcre2_jit_compile.c
	${PCRE_INCLUDE_DIR}/pcre2_match.c
	${PCRE_INCLUDE_DIR}/pcre2_match_data.c
	${PCRE_INCLUDE_DIR}/pcre2_newline.c
	${PCRE_INCLUDE_DIR}/pcre2_ord2utf.c
	${PCRE_INCLUDE_DIR}/pcre2_script_run.c
	${PCRE_INCLUDE_DIR}/pcre2_string_utils.c
	${PCRE_INCLUDE_DIR}/pcre2_study.c
	${PCRE_INCLUDE_DIR}/pcre2_tables.c
	${PCRE_INCLUDE_DIR}/pcre2_ucd.c
	${PCRE_INCLUDE_DIR}/pcre2_valid_utf.c
	${PCRE_INCLUDE_DIR}/pcre2_xclass.c
    ${CMAKE_CURRENT_BINARY_DIR}/pcre2/src/pcre2_chartables.c
)
source_group ("libs\\pcre" FILES ${pcre_sources})

if (NOT TARGET pcre)
	ccl_add_library (pcre INTERFACE)
	target_sources (pcre INTERFACE ${pcre_sources})
	target_include_directories (pcre INTERFACE ${PCRE_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/pcre2/include)
	target_compile_definitions (pcre INTERFACE HAVE_CONFIG_H PCRE2_STATIC=1 PCRE2_CODE_UNIT_WIDTH=16 SUPPORT_PCRE2_16=1 SUPPORT_UNICODE=1)
endif ()

set (PCRE_LIBRARY pcre)

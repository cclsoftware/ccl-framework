list (APPEND sqlite_lib_sources
    ${sqlite_lib_sourcedir}/mutex_unix.c
	${sqlite_lib_sourcedir}/os_unix.c
)

source_group ("source\\sqlite" FILES ${sqlite_lib_sources})

target_compile_options (sqlitelib INTERFACE "-Wno-int-to-pointer-cast;-Wno-pointer-to-int-cast;-Wno-unused-result;-Wno-unused-value;-Wno-missing-declarations;-Wno-shorten-64-to-32")

list (APPEND sqlite_lib_sources
    ${sqlite_lib_sourcedir}/mutex_unix.c
	${sqlite_lib_sourcedir}/os_unix.c
)

source_group ("source\\sqlite" FILES ${sqlite_lib_sources})

set_source_files_properties (${sqlite_lib_sources} PROPERTIES
    COMPILE_OPTIONS "-Wno-shorten-64-to-32;-Wno-unused-command-line-argument"
)

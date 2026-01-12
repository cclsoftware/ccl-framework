list (APPEND sqlite_lib_sources
    ${sqlite_lib_sourcedir}/mutex_w32.c
    ${sqlite_lib_sourcedir}/os_win.c
)

source_group ("source\\sqlite" FILES ${sqlite_lib_sources})

set_source_files_properties (${sqlite_lib_sources} PROPERTIES
    COMPILE_OPTIONS "/wd4244;/wd4267;/wd4018;/wd4311;/wd4133;/wd4312;/W2"
)

list (APPEND expat_SOURCES
    ${expat_DIR}/lib/random_arc4random_buf.c
    ${expat_DIR}/lib/random_arc4random_buf.h
)
source_group ("libs\\expat" FILES ${expat_SOURCES})

set_source_files_properties (${expat_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wno-shorten-64-to-32")

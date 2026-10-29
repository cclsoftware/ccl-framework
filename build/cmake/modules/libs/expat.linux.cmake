include (CheckCSourceCompiles)

check_c_source_compiles ("
    #include <stdlib.h>
    int main (void) {
      char dummy[123];
      arc4random_buf (dummy, 0U);
      return 0;
    }"
    HAVE_ARC4RANDOM_BUF)

check_c_source_compiles ("
    #include <unistd.h>
    int main (void) {
      return getentropy (NULL, 0U);
    }"
    HAVE_GETENTROPY)

if (HAVE_ARC4RANDOM_BUF)
    list (APPEND expat_SOURCES
        ${expat_DIR}/lib/random_arc4random_buf.c
        ${expat_DIR}/lib/random_arc4random_buf.h
    )
elseif (HAVE_GETENTROPY)
    list (APPEND expat_SOURCES
        ${expat_DIR}/lib/random_getentropy.c
        ${expat_DIR}/lib/random_getentropy.h
    )
endif ()

source_group ("libs\\expat" FILES ${expat_SOURCES})

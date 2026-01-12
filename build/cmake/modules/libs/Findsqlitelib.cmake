include_guard (DIRECTORY)

ccl_find_path (sqlite_lib_sourcedir NAMES "sqlite3.h" HINTS "${CCL_SUBMODULES_DIR}/sqlite" DOC "SQLite lib source directory")
mark_as_advanced (sqlite_lib_sourcedir)

list (APPEND sqlite_lib_sources
    ${sqlite_lib_sourcedir}/alter.c
    ${sqlite_lib_sourcedir}/analyze.c
    ${sqlite_lib_sourcedir}/attach.c
    ${sqlite_lib_sourcedir}/auth.c
    ${sqlite_lib_sourcedir}/backup.c
    ${sqlite_lib_sourcedir}/bitvec.c
    ${sqlite_lib_sourcedir}/btmutex.c
    ${sqlite_lib_sourcedir}/btree.c
    ${sqlite_lib_sourcedir}/build.c
    ${sqlite_lib_sourcedir}/callback.c
    ${sqlite_lib_sourcedir}/carray.c
    ${sqlite_lib_sourcedir}/complete.c
    ${sqlite_lib_sourcedir}/ctime.c
    ${sqlite_lib_sourcedir}/date.c
    ${sqlite_lib_sourcedir}/delete.c
    ${sqlite_lib_sourcedir}/expr.c
    ${sqlite_lib_sourcedir}/fault.c
    ${sqlite_lib_sourcedir}/fkey.c
    ${sqlite_lib_sourcedir}/func.c
    ${sqlite_lib_sourcedir}/global.c
    ${sqlite_lib_sourcedir}/hash.c
    ${sqlite_lib_sourcedir}/insert.c
    ${sqlite_lib_sourcedir}/json.c
    ${sqlite_lib_sourcedir}/legacy.c
    ${sqlite_lib_sourcedir}/loadext.c
    ${sqlite_lib_sourcedir}/malloc.c
    ${sqlite_lib_sourcedir}/main.c
    ${sqlite_lib_sourcedir}/mem1.c
    ${sqlite_lib_sourcedir}/memdb.c
    ${sqlite_lib_sourcedir}/memjournal.c
    ${sqlite_lib_sourcedir}/mutex.c
    ${sqlite_lib_sourcedir}/mutex_noop.c
    ${sqlite_lib_sourcedir}/opcodes.c
    ${sqlite_lib_sourcedir}/os.c
    ${sqlite_lib_sourcedir}/pager.c
    ${sqlite_lib_sourcedir}/parse.c
    ${sqlite_lib_sourcedir}/pcache.c
    ${sqlite_lib_sourcedir}/pcache1.c
    ${sqlite_lib_sourcedir}/pragma.c
    ${sqlite_lib_sourcedir}/prepare.c
    ${sqlite_lib_sourcedir}/printf.c
    ${sqlite_lib_sourcedir}/random.c
    ${sqlite_lib_sourcedir}/resolve.c
    ${sqlite_lib_sourcedir}/rowset.c
    ${sqlite_lib_sourcedir}/select.c
    ${sqlite_lib_sourcedir}/status.c
    ${sqlite_lib_sourcedir}/table.c
    ${sqlite_lib_sourcedir}/threads.c
    ${sqlite_lib_sourcedir}/tokenize.c
    ${sqlite_lib_sourcedir}/trigger.c
    ${sqlite_lib_sourcedir}/update.c
    ${sqlite_lib_sourcedir}/upsert.c
    ${sqlite_lib_sourcedir}/utf.c
    ${sqlite_lib_sourcedir}/util.c
    ${sqlite_lib_sourcedir}/vacuum.c
    ${sqlite_lib_sourcedir}/vdbe.c
    ${sqlite_lib_sourcedir}/vdbeapi.c
    ${sqlite_lib_sourcedir}/vdbeaux.c
    ${sqlite_lib_sourcedir}/vdbeblob.c
    ${sqlite_lib_sourcedir}/vdbemem.c
    ${sqlite_lib_sourcedir}/vdbesort.c
    ${sqlite_lib_sourcedir}/vdbetrace.c
    ${sqlite_lib_sourcedir}/vtab.c
    ${sqlite_lib_sourcedir}/wal.c
    ${sqlite_lib_sourcedir}/walker.c
    ${sqlite_lib_sourcedir}/where.c
    ${sqlite_lib_sourcedir}/wherecode.c
    ${sqlite_lib_sourcedir}/whereexpr.c
    ${sqlite_lib_sourcedir}/window.c
)

source_group ("source\\sqlite" FILES ${sqlite_lib_sources})

if (NOT TARGET sqlitelib)
	ccl_add_library (sqlitelib INTERFACE)
	target_sources (sqlitelib INTERFACE ${sqlite_lib_sources})
	target_include_directories (sqlitelib INTERFACE ${sqlite_lib_sourcedir})
endif ()

set (SQLITE_LIBRARY sqlitelib)


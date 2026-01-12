include_guard (DIRECTORY) 

set_target_properties(jsengine PROPERTIES INSTALL_RPATH "$ORIGIN/..;${CMAKE_INSTALL_RPATH}")

target_compile_definitions (jsengine PRIVATE OSATOMIC_USE_INLINED=1 HAVE_THREAD_TLS_KEYWORD=1)


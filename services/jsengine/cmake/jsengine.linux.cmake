include_guard (DIRECTORY) 

target_compile_definitions (jsengine PRIVATE OSATOMIC_USE_INLINED=1 HAVE_THREAD_TLS_KEYWORD=1)


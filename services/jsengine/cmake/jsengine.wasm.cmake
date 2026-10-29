include_guard (DIRECTORY)

# QuickJS-ng's own soft stack-overflow check assumes a 1 MB stack unless told otherwise
target_compile_definitions (jsengine PRIVATE JSENGINE_STACK_SIZE=512000) # 500 KB

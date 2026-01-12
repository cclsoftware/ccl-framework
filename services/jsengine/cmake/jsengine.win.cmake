target_compile_options(jsengine
  PRIVATE "/utf-8"
  "/Zc:preprocessor"
)

target_link_options(jsengine 
  PRIVATE "/ignore:4217"
  "$<$<CONFIG:DEBUG>:/NODEFAULTLIB:\"MSVCRT\">"
)

include ("${CMAKE_CURRENT_LIST_DIR}/miniaudio.mac.cmake")
target_link_libraries (miniaudio INTERFACE "-framework AVFoundation")

# On iOS miniaudio uses AVFoundation, so the implementation must be compiled as Objective-C.
set_source_files_properties ("${miniaudio_DIR}/miniaudio.c" PROPERTIES XCODE_EXPLICIT_FILE_TYPE "sourcecode.c.objc")

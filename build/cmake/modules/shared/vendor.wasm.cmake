include_guard (DIRECTORY)

include (vendor.clang)

option (VENDOR_WASM_USE_PTHREADS "Enable Emscripten pthreads and SharedArrayBuffer support (requires the app to be served with cross-origin isolation headers)" ON)

# add default definitions
add_compile_definitions (
	"$<$<CONFIG:DEBUG>:DEBUG=1;_DEBUG=1>"
	"$<$<CONFIG:RELEASE>:NDEBUG=1>"
)

# add default compiler options
add_compile_options (
	"-fno-delete-null-pointer-checks"					# don't treat usage of null pointers as undefined behavior
	"$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>"				# we don't use rtti
	"$<$<COMPILE_LANGUAGE:CXX>:-fsized-deallocation>"	# enable sized deallocation for C++11 (is default for C++14 and above)
	"-ffunction-sections"								# strip unused functions
	"-fdata-sections"									# strip unused data
)

if (VENDOR_WASM_USE_PTHREADS)
	add_compile_definitions ("_PTHREADS=1;NVWA_PTHREADS=1")	# nvwa debug malloc checks for these macros
	add_compile_options ("-pthread")
endif ()

# add default linker options
add_link_options (
	"$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>"				# we don't use rtti
)

string (APPEND CMAKE_SHARED_LINKER_FLAGS
	"-Wl,--gc-sections"									# strip unused symbols, see -ffunction-sections and -fdata-sections
)

if (VENDOR_ENABLE_DEBUG_SYMBOLS)
	cmake_path (RELATIVE_PATH VENDOR_OUTPUT_DIRECTORY BASE_DIRECTORY "${REPOSITORY_ROOT}" OUTPUT_VARIABLE relative_output_dir)
	add_compile_options (
		"-g0;-gsource-map"								# generate source maps instead of embedded debug info
	)
	add_link_options (
		"-g0;-gsource-map"
		"--source-map-base=http://localhost:8080/${relative_output_dir}/${VENDOR_PLATFORM}/$<CONFIG>/"
	)
endif ()

option (ENABLE_STACK_PROTECTION "Enable Stack Protection" OFF)
if (${ENABLE_STACK_PROTECTION})
	add_compile_options (
		"-fno-omit-frame-pointer"
		"-fstack-protector-all"
		"-fsanitize=safe-stack"
	)
	add_link_options (
		"-fno-omit-frame-pointer"
		"-Wl,--allow-shlib-undefined"
		"-fstack-protector-all"
		"-fsanitize=safe-stack"
	)
endif ()

if (VENDOR_DYNAMIC_LINKER_PATH)
	add_link_options ("-Wl,-dynamic-linker,${VENDOR_DYNAMIC_LINKER_PATH}")
endif ()

set (VENDOR_EMSCRIPTEN_FLAGS "")
if (VENDOR_WASM_USE_PTHREADS)
	set (VENDOR_EMSCRIPTEN_FLAGS "-sUSE_PTHREADS=1 -sSHARED_MEMORY=1")
endif ()
set (VENDOR_EMSCRIPTEN_LINKER_FLAGS "${VENDOR_EMSCRIPTEN_FLAGS} -sSTACK_SIZE=1mb -sALLOW_MEMORY_GROWTH=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sLLD_REPORT_UNDEFINED")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	string (APPEND VENDOR_EMSCRIPTEN_LINKER_FLAGS " -sASSERTIONS=2") # -sVERBOSE=1 -sSAFE_HEAP=1")
endif ()

if (VENDOR_HTML_SHELL_FILE)
	set (CCL_HTML_SHELL_FILE "${VENDOR_HTML_SHELL_FILE}" CACHE FILEPATH "" FORCE)
else ()
	find_file (ccl_shell_file NAMES shell.html HINTS "${CCL_REPOSITORY_ROOT}/ccl/packaging/wasm/resource/")
	set (CCL_HTML_SHELL_FILE "${ccl_shell_file}" CACHE FILEPATH "" FORCE)
endif ()
if (CCL_HTML_SHELL_FILE)
	set (CMAKE_EXECUTABLE_SUFFIX ".html")
endif ()

set_property (GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
set (CMAKE_SHARED_MODULE_SUFFIX ".so")
set (CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "${VENDOR_EMSCRIPTEN_FLAGS}")
set (CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${VENDOR_EMSCRIPTEN_FLAGS}")
set (CMAKE_SHARED_LINKER_FLAGS "-sSIDE_MODULE=2")
set (CMAKE_MODULE_LINKER_FLAGS "-sSIDE_MODULE=2")

set (CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)
set (CMAKE_VISIBILITY_INLINES_HIDDEN ON CACHE BOOL "" FORCE)
set (CMAKE_CXX_VISIBILITY_PRESET hidden)
set (CMAKE_C_VISIBILITY_PRESET hidden)

set (CMAKE_CONFIGURATION_TYPES "Release;Debug")

set (CMAKE_INSTALL_RPATH "$ORIGIN")
set (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set (CMAKE_INSTALL_LIBDIR "lib")
set (CMAKE_INSTALL_INCLUDEDIR "include")
set (CMAKE_INSTALL_BINDIR "bin")

set (CMAKE_SHARED_MODULE_PREFIX "")

set (VENDOR_TARGET_ARCHITECTURE "wasm")

if (NOT DEFINED CPACK_GENERATOR)
	set (CPACK_GENERATOR ZIP)
endif ()

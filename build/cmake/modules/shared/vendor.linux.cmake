include_guard (DIRECTORY)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	include (vendor.clang)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message (NOTICE "Clang is the recommended compiler on Linux. Consider running cmake with -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_C_COMPILER=/usr/bin/clang")
	include (vendor.gcc)
endif ()

set (VENDOR_LTO_CACHE_DIRECTORY "${REPOSITORY_ROOT}/build/cmake/linux/cache/lto" CACHE PATH "Link time optimization cache directory")
set (VENDOR_LTO_CACHE_SIZE "2g" CACHE STRING "Link time optimization cache size")

# add default definitions
add_compile_definitions (
	"_PTHREADS=1;NVWA_PTHREADS=1"						# nvwa debug malloc checks for these macros
	"$<$<CONFIG:DEBUG>:DEBUG=1;_DEBUG=1>"
	"$<$<CONFIG:RELEASE>:NDEBUG=1>"
)

# add default compiler options
add_compile_options (
	"-fno-delete-null-pointer-checks"					# don't treat usage of null pointers as undefined behavior
	"$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>"				# we don't use RTTI
	"$<$<COMPILE_LANGUAGE:CXX>:-fsized-deallocation>"	# enable sized deallocation for C++11 (is default for C++14 and above)
	"-ffunction-sections"								# strip unused functions
	"-fdata-sections"									# strip unused data
)

if (VENDOR_ENABLE_LINK_TIME_OPTIMIZATION)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		add_compile_options ("$<$<CONFIG:RELEASE>:-flto=thin>")	# incremental link time optimization
	endif ()
endif ()

if (VENDOR_ENABLE_DEBUG_SYMBOLS)
	add_compile_options ("-g;-ggdb")					# always add debug symbols, we'll strip them in the packaging step
endif ()

if ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(arm64)")
	add_compile_options ("-march=armv8-a")
endif ()

# add default linker options
add_link_options (
	"-fuse-ld=lld"										# use lld linker
	"-Wl,--build-id=sha1"								# add a build ID, so we can find the right symbols when resolving crash dumps
	"-Wl,--no-allow-shlib-undefined"					# don't allow unresolved symbols in shared libraries, i.e. show an error at link time, not at runtime
	"$<$<CONFIG:RELEASE>:-Wl,-z,relro,-z,now>"			# hardened binaries
	#"$<$<CONFIG:RELEASE>:-Wl,--thinlto-jobs=2>"		# link time optimization uses two threads
	"-Wl,--thinlto-cache-dir=${VENDOR_LTO_CACHE_DIRECTORY}"	# link time optimization cache
	"-Wl,--thinlto-cache-policy,cache_size_bytes=${VENDOR_LTO_CACHE_SIZE}"	# limit lto cache size
	"-Wl,--compress-debug-sections=zlib"				# compress debug symbols
	"-Wl,--no-as-needed"								# keep all explicit dependencies
)

if (VENDOR_ENABLE_LINK_TIME_OPTIMIZATION)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		add_link_options ("$<$<CONFIG:RELEASE>:-flto=thin>")	# enable link time optimization for release builds
	endif ()
endif ()

string (APPEND CMAKE_SHARED_LINKER_FLAGS
	" -Wl,--gc-sections"								# strip unused symbols, see -ffunction-sections and -fdata-sections
)

option (ENABLE_ASAN "Enable Address Sanitizer" OFF)
if (${ENABLE_ASAN})
	add_compile_options (
		"-fno-omit-frame-pointer"
		"-fsanitize=address"
	)
	add_link_options (
		"-fno-omit-frame-pointer"
		"-fsanitize=address"
		"-Wl,--allow-shlib-undefined"
	)
	add_compile_definitions (
		"__SANITIZE_ADDRESS__=1"
	)
else ()
	add_link_options ("-Wl,--exclude-libs,ALL")			# hide exported symbols when linking static libraries
endif ()

option (ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)
if (${ENABLE_UBSAN})
	add_compile_options (
		"-fno-omit-frame-pointer"
		"-fsanitize=undefined"
		"-fno-sanitize=vptr" # not compatible with -fno-rtti
	)
	add_link_options (
		"-fno-omit-frame-pointer"
		"-fsanitize=undefined"
		"-fno-sanitize=vptr" # not compatible with -fno-rtti
		"-Wl,--allow-shlib-undefined"
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

option (ENABLE_PROFILING "Enable profiling" OFF)
if (${ENABLE_PROFILING})
	add_compile_options ("-pg")
	add_link_options ("-pg")
endif ()

if (CMAKE_CROSSCOMPILING)
	add_link_options ("-Wl,--allow-shlib-undefined")
endif ()

if (VENDOR_DYNAMIC_LINKER_PATH)
	add_link_options ("-Wl,-dynamic-linker,${VENDOR_DYNAMIC_LINKER_PATH}")
endif ()

set (CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)
set (CMAKE_VISIBILITY_INLINES_HIDDEN ON CACHE BOOL "" FORCE)
set (CMAKE_CXX_VISIBILITY_PRESET hidden)
set (CMAKE_C_VISIBILITY_PRESET hidden)

set (CMAKE_CONFIGURATION_TYPES "Release;Debug")

set (CMAKE_INSTALL_RPATH "$ORIGIN")
set (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set (CMAKE_SHARED_MODULE_PREFIX "")

# Packaging
set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set (CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)
if ("${VENDOR_TARGET_ARCHITECTURE}" MATCHES "(x86_64)")
	set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
else ()
	set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
endif ()

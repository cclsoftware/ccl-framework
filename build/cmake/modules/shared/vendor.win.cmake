include_guard (DIRECTORY)

# detect target architecture
if ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "i386")
	set (VENDOR_PLATFORM_SUBDIR "x86")
elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "x86_64")
	set (VENDOR_PLATFORM_SUBDIR "x64")
	add_compile_definitions (_WIN64)
elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64")
	set (VENDOR_PLATFORM_SUBDIR "arm64")
	add_compile_definitions (_WIN64)
elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64ec" AND NOT VENDOR_BUILD_AS_ARM64X)
	set (VENDOR_PLATFORM_SUBDIR "arm64ec")
	add_compile_definitions (_WIN64)
elseif ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "arm64ec")
	set (VENDOR_PLATFORM_SUBDIR "arm64x")
	add_compile_definitions (_WIN64)
else ()
	set (VENDOR_PLATFORM_SUBDIR "unknown")
endif ()

# add default definitions
add_compile_definitions (WIN32 _CRT_SECURE_NO_WARNINGS)
add_compile_definitions (_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR=1)

if (MSVC)
	# remove unwanted default compiler options
	foreach (var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
		string (REGEX REPLACE "/RTC[^ ]*" "" ${var} "${${var}}")	# disable runtime error checks
		string (REGEX REPLACE "/Ob[^ ]*" "" ${var} "${${var}}")		# default inline function expansion
		string (REGEX REPLACE "/Fd[^ ]*" "" ${var} "${${var}}")		# default program database file name
	endforeach ()

	# add default compiler options
	add_compile_options (
		"$<$<COMPILE_LANGUAGE:C,CXX>:/GS->"				# disable buffer security checks
		"$<$<COMPILE_LANGUAGE:C,CXX>:/Oi>"				# generate instrinsic functions
		"$<$<COMPILE_LANGUAGE:C,CXX>:/fp:fast>"			# fast floating point model
		"$<$<COMPILE_LANGUAGE:C,CXX>:/FC>"				# display the full path of source code files in diagnostics
		"$<$<COMPILE_LANGUAGE:C,CXX>:/GF>"				# eliminate duplicate strings
		"$<$<COMPILE_LANGUAGE:C,CXX>:/Gy>"				# enable function-level linking
		"$<$<COMPILE_LANGUAGE:CXX>:/EHa>"				# catch both structured (asynchronous) and standard C++ (synchronous) exceptions
		"$<$<COMPILE_LANGUAGE:CXX>:/GR->"				# disable RTTI

		"$<$<COMPILE_LANGUAGE:C,CXX>:/wd4996>"			# disable warning: "Function call with parameters that may be unsafe"
		"$<$<COMPILE_LANGUAGE:C,CXX>:/wd4355>"			# disable warning: "'this' : used in base member initializer list"
		"$<$<COMPILE_LANGUAGE:C,CXX>:/wd4316>"			# disable warning: "Object allocated on the heap may not be aligned for this type"
		"$<$<COMPILE_LANGUAGE:C,CXX>:/wd4267>"			# disable warning: "'var' : conversion from 'size_t' to 'type', possible loss of data"
		"$<$<COMPILE_LANGUAGE:C,CXX>:/wd4244>"			# disable warning: "conversion from 'int' to 'float', possible loss of data"
		"$<$<COMPILE_LANGUAGE:C,CXX>:/wd4018>"			# disable warning: "signed/unsigned mismatch"
		"$<$<COMPILE_LANGUAGE:CXX>:/wd4091>"			# disable warning: "typedef struct ignored when no variable is declared"

		"$<$<COMPILE_LANGUAGE:C,CXX>:/we4541>"			# warning as error: "'identifier' used on polymorphic type 'type' with /GR-; unpredictable behavior may result"
		"$<$<COMPILE_LANGUAGE:C,CXX>:/we4715>"			# warning as error: "not all control paths return a value"

		"$<$<COMPILE_LANGUAGE:C,CXX>:/w34242>"			# warning level 3: "'identifier': conversion from 'type1' to 'type2', possible loss of data"
		"$<$<COMPILE_LANGUAGE:C,CXX>:/w34254>"			# warning level 3: "'operator': conversion from 'type1' to 'type2', possible loss of data"
		"$<$<COMPILE_LANGUAGE:C,CXX>:/w34431>"			# warning level 3: "missing type specifier - int assumed. Note: C no longer supports default-int"
		"$<$<COMPILE_LANGUAGE:C,CXX>:/w34296>"			# warning level 3: "unsigned variable was used in a comparison operation with zero"

		"/W3"											# warning level 3
		"$<$<COMPILE_LANGUAGE:C,CXX>:$<$<CONFIG:DEBUG>:/Od;/JMC>>" # turn off all optimizations, just my code debugging
		"$<$<COMPILE_LANGUAGE:C,CXX>:$<$<CONFIG:RELEASE>:/fp:except-;/Ob2;/O2;/Ot>>" # floating point exceptions, inline function expansion, optimization level, favor fast code
	)
	
	if (VENDOR_ENABLE_LINK_TIME_OPTIMIZATION)
		add_compile_options ("$<$<COMPILE_LANGUAGE:C,CXX>:$<$<CONFIG:RELEASE>:/GL>>")	# whole program optimization
	endif ()

	option (ENABLE_ASAN "Enable Address Sanitizer" OFF)
	if (ENABLE_ASAN)
		add_compile_options ("/fsanitize=address")
		add_compile_definitions ("__SANITIZE_ADDRESS__=1")
	endif ()
	
	option (ENABLE_CODE_ANALYSIS "Enable Code Analysis" OFF)

	# when using ccache or asan, disable unsupported compiler options
	# see https://github.com/ccache/ccache/issues/1040
	if (NOT VENDOR_USE_CCACHE)
		add_compile_options (
			"$<$<COMPILE_LANGUAGE:C,CXX>:/MP>"						# multi-process compilation, not supported by ccache
		)
	endif ()
	if (NOT VENDOR_USE_CCACHE AND NOT ENABLE_ASAN)
		add_compile_options (
			"$<$<COMPILE_LANGUAGE:C,CXX>:$<$<CONFIG:DEBUG>:/ZI>>" 	# debug information format: separate PDB with edit and continue, not supported by ccache or asan
			"$<$<COMPILE_LANGUAGE:C,CXX>:$<$<CONFIG:RELEASE>:/Zi>>" 	# debug information format: separate PDB, not supported by ccache or asan
		)
	else ()
		add_compile_options ("$<$<COMPILE_LANGUAGE:C,CXX>:$<$<CONFIG:DEBUG>:/Z7>>") # full symbolic debugging information
	endif ()

	# add default linker options
	add_link_options (
		"/LARGEADDRESSAWARE"
		"/NODEFAULTLIB:\"LIBCMT\""
		"/DEBUG"
		"/ignore:4099"    #PDB 'filename' was not found with 'object/library'; linking object as if no debug info
		"$<$<CONFIG:DEBUG>:/SAFESEH:NO>"
		"$<$<CONFIG:RELEASE>:/OPT:REF;/OPT:ICF>"
	)

	if (VENDOR_ENABLE_LINK_TIME_OPTIMIZATION)
		add_link_options ("$<$<CONFIG:RELEASE>:/LTCG>")	# link time code generation
	endif ()

	# Windows ARM64 makes DYNAMICBASE and NXCOMPAT mandatory
	if (NOT "${VENDOR_TARGET_ARCHITECTURE}" MATCHES "arm64")
		add_link_options (
			"/DYNAMICBASE:NO"
			"/NXCOMPAT:NO"
		)
	endif ()
	
endif ()

# these are the only configurations we need in VS
set (CMAKE_CONFIGURATION_TYPES "Release;Debug")

set (CMAKE_INSTALL_BINDIR "" CACHE PATH "" FORCE)
set (CMAKE_INSTALL_LIBDIR "" CACHE PATH "" FORCE)

# find libraries in pragma directives
link_directories ("${REPOSITORY_ROOT}")

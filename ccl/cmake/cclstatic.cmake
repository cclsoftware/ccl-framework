#************************************************************************************************
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2025 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
# 
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : cclstatic.cmake
# Description : CCL static library
#
#************************************************************************************************

include_guard (DIRECTORY)

# Find dependencies
find_package (corelib REQUIRED)
find_package (cryptlib)

# Add targets
if (NOT TARGET cclstatic)
	ccl_add_library (cclstatic STATIC VENDOR ccl)
endif ()
list (APPEND CCL_IS_STATIC cclstatic)
set_target_properties (cclstatic PROPERTIES FOLDER "ccl")

# collect source files
list (APPEND CMAKE_MODULE_PATH "${CCL_DIR}/cmake")
foreach (target IN ITEMS ${ccl_FIND_COMPONENTS})
	if (target MATCHES "^cclextras*")
		continue ()
	endif ()
	set (listfile "${CCL_DIR}/cmake/${target}.cmake")
	if (EXISTS ${listfile})
		include ("${target}")
		list (FIND CCL_STATIC_LIBRARIES ${target} is_static_library)
		if (${is_static_library} EQUAL -1)
			get_target_property (target_sources ${target} SOURCES)
			if (target_sources)
				list (APPEND cclstatic_sources ${target_sources})
			endif ()
		endif ()
		list (APPEND cclstatic_sources "${listfile}")
		source_group ("cmake" FILES "${listfile}")
	endif ()
endforeach ()

LIST (APPEND cclstatic_interface_sources
	${CCL_DIR}/main/cclstatic.cpp
	${CCL_DIR}/main/cclstatic.h
)

foreach (item IN ITEMS ${cclstatic_sources})
	string (FIND "${item}" "ccl/main/" string_main)
	string (FIND "${item}" "cclsystemmain.cpp" string_systemmain)
	string (FIND "${item}" ".def" string_def)
	string (FIND "${item}" ".rc" string_rc)
	if (NOT ${string_main} EQUAL -1 OR NOT ${string_def} EQUAL -1 OR NOT ${string_rc} EQUAL -1 OR NOT ${string_systemmain} EQUAL -1)
		list (REMOVE_ITEM cclstatic_sources "${item}")
	endif ()
endforeach ()

source_group ("source\\ccl" FILES ${cclstatic_interface_sources})
source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})

target_sources (cclstatic PRIVATE ${cclstatic_sources} ${CMAKE_CURRENT_LIST_FILE} INTERFACE ${cclstatic_interface_sources})

target_include_directories (cclstatic PUBLIC ${ZLIB_INCLUDE_DIRS} ${CCL_INCLUDE_DIRS} ${CCL_STATIC_INCLUDE_DIRS})
target_compile_definitions (cclstatic PUBLIC CCL_STATIC_LINKAGE=1 ${CCL_STATIC_COMPILE_DEFINITIONS})
target_compile_options (cclstatic PRIVATE ${CCL_STATIC_COMPILE_OPTIONS})
target_link_options (cclstatic PUBLIC ${CCL_STATIC_LINK_OPTIONS})
target_link_libraries (cclstatic PUBLIC ${CCL_STATIC_LINK_LIBRARIES} cryptlib corelib)


find_library (dnssd NAMES dns_sd REQUIRED)

ccl_check_imported (${cclnet} imported)
if (NOT imported)
	if (NOT ${CCL_STATIC_ONLY})
		target_link_libraries (${cclnet} PRIVATE ${dnssd})
		target_include_directories (${cclnet} PRIVATE ${dnssd_INCLUDE_DIR})
	endif ()
endif ()
list (APPEND CCL_STATIC_LINK_LIBRARIES ${dnssd})
list (APPEND CCL_STATIC_INCLUDE_DIRS "${dnssd_INCLUDE_DIR}")

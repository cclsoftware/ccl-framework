if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
	target_link_directories (${cclnet} PUBLIC $<BUILD_INTERFACE:${CCL_REPOSITORY_ROOT}>)
endif ()

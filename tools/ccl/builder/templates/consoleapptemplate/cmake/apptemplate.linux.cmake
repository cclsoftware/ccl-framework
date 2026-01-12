include_guard (DIRECTORY)

list (APPEND apptemplate_ccl_sources
	${CCL_DIR}/platform/shared/posix/posixconsolemain.cpp
)

install (TARGETS apptemplate RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}")


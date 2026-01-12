include_guard (DIRECTORY) 

set_target_properties (servicetemplate PROPERTIES INSTALL_RPATH "$ORIGIN/..")

install (TARGETS servicetemplate LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})


include_guard (DIRECTORY) 

set_target_properties (firebaseservice PROPERTIES INSTALL_RPATH "$ORIGIN/..")

install (TARGETS firebaseservice LIBRARY DESTINATION ${VENDOR_PLUGINS_RUNTIME_DIRECTORY})


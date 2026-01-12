
# disable warning 4242 is ignored when this warning's level is set to 3 in 'vendor.win.cmake'
# so we just lower the warning level for these specific files
set_source_files_properties (${zlib_sources} PROPERTIES
    COMPILE_FLAGS "/W2"
)
include_guard (DIRECTORY)

find_library (COREFOUNDATION_LIBRARY CoreFoundation)
find_library (IOKIT_LIBRARY IOKit)
find_library (CARBON_LIBRARY Carbon)
find_library (FOUNDATION_LIBRARY Foundation)
find_library (APPKIT_LIBRARY AppKit)
find_library (SECURITY_LIBRARY Security)
find_library (AUDIOTOOLBOX_LIBRARY AudioToolbox)

target_link_libraries (cclstatic INTERFACE ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY} ${CARBON_LIBRARY} ${FOUNDATION_LIBRARY} ${APPKIT_LIBRARY} ${SECURITY_LIBRARY} ${AUDIOTOOLBOX_LIBRARY})

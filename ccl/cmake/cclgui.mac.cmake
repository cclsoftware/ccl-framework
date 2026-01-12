ccl_list_append_once (cclgui_cocoa_sources
	${CCL_DIR}/platform/cocoa/gui/accessibility.mac.mm
	${CCL_DIR}/platform/cocoa/gui/accessibility.mac.h
	${CCL_DIR}/platform/cocoa/gui/fontresource.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/alert.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/transparentwindow.cocoa.h
	${CCL_DIR}/platform/cocoa/gui/transparentwindow.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/clipboard.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/platformwindow.mac.h
	${CCL_DIR}/platform/cocoa/gui/platformwindow.mac.mm
	${CCL_DIR}/platform/cocoa/gui/fileselector.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/tooltip.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/cagraphicslayer.cocoa.h
	${CCL_DIR}/platform/cocoa/gui/cagraphicslayer.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/notifyicon.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/theme.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/exceptionhandler.h
	${CCL_DIR}/platform/cocoa/gui/exceptionhandler.mm
	${CCL_DIR}/platform/cocoa/gui/mousecursor.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/nativegraphics.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/documentviewer.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/webbrowserview.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/legacywebbrowserview.mac.h
	${CCL_DIR}/platform/cocoa/gui/legacywebbrowserview.mac.mm
	${CCL_DIR}/platform/cocoa/gui/nativeview.mac.h
	${CCL_DIR}/platform/cocoa/gui/nativeview.mac.mm
	${CCL_DIR}/platform/cocoa/gui/gui.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/desktop.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/printjob.cocoa.h
	${CCL_DIR}/platform/cocoa/gui/printjob.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/dialog.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/keyevent.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/window.mac.h
	${CCL_DIR}/platform/cocoa/gui/window.mac.mm
	${CCL_DIR}/platform/cocoa/gui/dragndrop.cocoa.h
	${CCL_DIR}/platform/cocoa/gui/dragndrop.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/systemshell.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/printservice.cocoa.h
	${CCL_DIR}/platform/cocoa/gui/printservice.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/menu.cocoa.h
	${CCL_DIR}/platform/cocoa/gui/menu.cocoa.mm
	${CCL_DIR}/platform/cocoa/gui/textbox.cocoa.mm
	${CCL_DIR}/platform/cocoa/interfaces/iquartzbitmap.h
	${CCL_DIR}/platform/cocoa/quartz/quartzbitmap.mm
	${CCL_DIR}/platform/cocoa/quartz/quartzbitmap.h
	${CCL_DIR}/platform/cocoa/quartz/cgdataprovider.cpp
	${CCL_DIR}/platform/cocoa/quartz/device.mm
	${CCL_DIR}/platform/cocoa/quartz/device.h
	${CCL_DIR}/platform/cocoa/quartz/engine.h
	${CCL_DIR}/platform/cocoa/quartz/engine.mm
	${CCL_DIR}/platform/cocoa/quartz/fontcache.mm
	${CCL_DIR}/platform/cocoa/quartz/gradient.mm
	${CCL_DIR}/platform/cocoa/quartz/gradient.h
	${CCL_DIR}/platform/cocoa/quartz/path.mm
	${CCL_DIR}/platform/cocoa/quartz/quartzlayer.mm
	${CCL_DIR}/platform/cocoa/quartz/quartzrendertarget.h
	${CCL_DIR}/platform/cocoa/quartz/quartzrendertarget.mm
	${CCL_DIR}/platform/cocoa/quartz/quartztextlayout.mm
	${CCL_DIR}/platform/cocoa/skia/skiabitmap.cocoa.h
	${CCL_DIR}/platform/cocoa/skia/skiabitmap.cocoa.mm
	${CCL_DIR}/platform/cocoa/skia/skiaengine.cocoa.h
	${CCL_DIR}/platform/cocoa/skia/skiaengine.cocoa.mm
	${CCL_DIR}/platform/cocoa/skia/skiarendertarget.cocoa.h
	${CCL_DIR}/platform/cocoa/skia/skiarendertarget.cocoa.mm
	${CCL_DIR}/platform/cocoa/skia/skiarendertarget.mac.h
	${CCL_DIR}/platform/cocoa/skia/skiarendertarget.mac.mm
	${CCL_DIR}/platform/cocoa/skia/skialayer.cocoa.h
	${CCL_DIR}/platform/cocoa/skia/skialayer.cocoa.mm
	${CCL_DIR}/platform/cocoa/metal/metal3dsupport.h
	${CCL_DIR}/platform/cocoa/metal/metal3dsupport.mm
	${CCL_DIR}/platform/cocoa/metal/metalclient.h
	${CCL_DIR}/platform/cocoa/metal/metalclient.mm
)

ccl_list_append_once (cclgui_platform_sources
	${CCL_DIR}/platform/shared/posix/gui/exceptionhandler.posix.cpp
	${CCL_DIR}/platform/shared/posix/gui/exceptionhandler.posix.h
	${CCL_DIR}/platform/shared/skia/skiastream.h
	${CCL_DIR}/platform/shared/skia/skiastream.cpp
	${CCL_DIR}/platform/shared/skia/skiabitmap.h
	${CCL_DIR}/platform/shared/skia/skiabitmap.cpp
	${CCL_DIR}/platform/shared/skia/skiadevice.h
	${CCL_DIR}/platform/shared/skia/skiadevice.cpp
	${CCL_DIR}/platform/shared/skia/skiaengine.h
	${CCL_DIR}/platform/shared/skia/skiaengine.cpp
	${CCL_DIR}/platform/shared/skia/skiafonttable.h
	${CCL_DIR}/platform/shared/skia/skiafonttable.cpp
	${CCL_DIR}/platform/shared/skia/skiagradient.h
	${CCL_DIR}/platform/shared/skia/skiagradient.cpp
	${CCL_DIR}/platform/shared/skia/skiapath.h
	${CCL_DIR}/platform/shared/skia/skiapath.cpp
	${CCL_DIR}/platform/shared/skia/skiarendertarget.h
	${CCL_DIR}/platform/shared/skia/skiarendertarget.cpp
	${CCL_DIR}/platform/shared/skia/skiatextlayout.h
	${CCL_DIR}/platform/shared/skia/skiatextlayout.cpp

	${CCL_DIR}/platform/shared/skia/test/skiatest.cpp
)

ccl_list_append_once (cclgui_sources
	${cclgui_cocoa_sources}
	${cclgui_platform_sources}
)

ccl_list_append_once (cclgui_metal_headers
	${CCL_DIR}/gui/graphics/3d/shader/metal/metalshader.h
	${CCL_DIR}/gui/graphics/3d/shader/metal/transform.h
	${CCL_DIR}/gui/graphics/3d/shader/metal/lighting.h
)

target_sources (${cclgui} PRIVATE  ${cclgui_metal_headers})

source_group (TREE ${CCL_DIR}/platform/cocoa PREFIX "source" FILES ${cclgui_cocoa_sources})
source_group (TREE ${CCL_DIR}/platform/shared PREFIX "source" FILES ${cclgui_platform_sources})
source_group ("resource/shaders" FILES ${cclgui_metal_headers})

find_library (METAL_LIBRARY Metal REQUIRED)
find_library (METALKIT_LIBRARY MetalKit REQUIRED)
find_library (COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
find_library (IOKIT_LIBRARY IOKit REQUIRED)
find_library (APPLICATIONSERVICES_LIBRARY ApplicationServices REQUIRED)
find_library (WEBKIT_LIBRARY WebKit REQUIRED)
find_library (FOUNDATION_LIBRARY Foundation REQUIRED)
find_library (QUARTZ_LIBRARY QuartzCore REQUIRED)
find_library (CORESERVICES_LIBRARY CoreServices REQUIRED)
find_library (COCOA_LIBRARY Cocoa REQUIRED)
find_library (CARBON_LIBRARY Carbon REQUIRED)
find_library (AUTHENTICATIONSERVICES_LIBRARY AuthenticationServices REQUIRED)
find_package (skia REQUIRED)

ccl_list_append_once (cclgui_apple_frameworks
	${METAL_LIBRARY}
	${METALKIT_LIBRARY}
	${COREFOUNDATION_LIBRARY}
	${IOKIT_LIBRARY}
	${APPLICATIONSERVICES_LIBRARY}
	${WEBKIT_LIBRARY}
	${FOUNDATION_LIBRARY}
	${QUARTZ_LIBRARY}
	${CORESERVICES_LIBRARY}
	${COCOA_LIBRARY}
	${CARBON_LIBRARY}
	${AUTHENTICATIONSERVICES_LIBRARY}
)

set_target_properties (${cclgui} PROPERTIES
	FRAMEWORK TRUE
)

target_compile_definitions (${cclgui} PRIVATE SK_GANESH=1 SK_METAL=1 SK_SHAPER_CORETEXT_AVAILABLE=1)

if (NOT ${CCL_STATIC_ONLY})
	target_link_libraries (${cclgui} PRIVATE ${SKIA_LIBRARIES} PUBLIC ${cclgui_apple_frameworks})
endif ()
ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${SKIA_LIBRARIES} ${cclgui_apple_frameworks})

ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/metal/vertexshaderPN.metal PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/metal/vertexshaderPNT.metal PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/metal/vertexshaderPNT2PN.metal PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/metal/billboardvertexshader.metal PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/metal/solidcolormaterial.metal PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/metal/texturematerial.metal PATH shaders)

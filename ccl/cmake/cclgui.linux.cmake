
if (CCL_ENABLE_VULKAN)
	if (Vulkan_LIBRARIES AND "${Vulkan_VERSION}" VERSION_GREATER_EQUAL "1.3.230" AND glslc_executable AND spirv_cross_executable)
		ccl_find_path (vulkan_enum_include_directory NAMES vulkan/vk_enum_string_helper.h HINTS /usr/include)
		if (vulkan_enum_include_directory)
			ccl_list_append_once (cclgui_graphics_includes ${vulkan_enum_include_directory})
		else ()
			find_package (vulkan-utility-libraries REQUIRED)
			ccl_list_append_once (cclgui_graphics_libraries ${Vulkan_Utility_Libraries_LIBRARY})
		endif ()
		ccl_list_append_once (cclgui_graphics_libraries ${Vulkan_LIBRARIES})
		ccl_list_append_once (cclgui_graphics_definitions SK_GANESH=1 SK_VULKAN=1 CCLGUI_VULKAN_ENABLED=1)
		ccl_list_append_once (skia_components vulkan)
	else ()
		message (NOTICE "Disabling Vulkan support. Missing dependencies.")
		set (CCL_ENABLE_VULKAN OFF CACHE BOOL "" Force)
		set (CCL_ENABLE_VULKAN OFF)
	endif ()
endif ()

if (CCL_ENABLE_OPENGLES2)
	find_package (OpenGL COMPONENTS EGL GLES2)
	if (OpenGL_FOUND)
		ccl_list_append_once (cclgui_graphics_libraries OpenGL::EGL OpenGL::GLES2)
		ccl_list_append_once (cclgui_graphics_includes ${OPENGL_INCLUDE_DIR} ${OPENGL_EGL_INCLUDE_DIRS})
		ccl_list_append_once (cclgui_graphics_definitions SK_GANESH=1 SK_GL=1 SK_ASSUME_GL_ES=1 CCLGUI_OPENGLES2_ENABLED=1)
		ccl_list_append_once (skia_components opengles2)
		ccl_list_append_once (wayland_components egl)
	else ()
		message (NOTICE "Disabling OpenGL ES 2 support. Missing dependencies.")
		set (CCL_ENABLE_OPENGLES2 OFF CACHE BOOL "" Force)
		set (CCL_ENABLE_OPENGLES2 OFF)
	endif ()
endif ()

ccl_use_dbus_interface (${cclgui} "${CMAKE_CURRENT_LIST_DIR}/../platform/linux/interfaces/dbus/dev.ccl.application.xml")
ccl_use_dbus_interface (${cclgui} "${CMAKE_CURRENT_LIST_DIR}/../platform/linux/interfaces/dbus/dev.ccl.application.xml" SERVER)

ccl_use_dbus_interface (${cclgui} org.freedesktop.portal.Request OPTIONAL)
ccl_use_dbus_interface (${cclgui} org.freedesktop.portal.Print OPTIONAL)
if (dbus_interface_FOUND)
	target_compile_definitions (${cclgui} PRIVATE CCLGUI_XDG_PRINTING_ENABLED=1)
	ccl_list_append_once (CCL_STATIC_COMPILE_DEFINITIONS CCLGUI_XDG_PRINTING_ENABLED=1)
	ccl_list_append_once (cclgui_linux_sources
		${CCL_DIR}/platform/linux/gui/xdgprintservice.cpp
		${CCL_DIR}/platform/linux/gui/xdgprintservice.h
	)
else ()
	message (NOTICE "Disabling printer support. Missing dependencies: xdg-desktop-portal.")

	if (NOT DEFINED CCL_BUILD_XDGPORTAL_INTEGRATION OR CCL_BUILD_XDGPORTAL_INTEGRATION)
		message (NOTICE "Disabling desktop integration using XDG Portal. Missing dependencies: xdg-desktop-portal.")
		set (CCL_BUILD_XDGPORTAL_INTEGRATION OFF CACHE BOOL "" Force)
		set (CCL_BUILD_XDGPORTAL_INTEGRATION OFF)
	endif ()
endif ()

ccl_list_append_once (cclgui_linux_sources
	${CCL_DIR}/platform/linux/gui/alert.linux.cpp
	${CCL_DIR}/platform/linux/gui/clipboard.linux.cpp
	${CCL_DIR}/platform/linux/gui/dbus.cpp
	${CCL_DIR}/platform/linux/gui/dbus.h
	${CCL_DIR}/platform/linux/gui/dbusapplication.cpp
	${CCL_DIR}/platform/linux/gui/dbusapplication.h
	${CCL_DIR}/platform/linux/gui/desktop.linux.cpp
	${CCL_DIR}/platform/linux/gui/dialog.linux.cpp
	${CCL_DIR}/platform/linux/gui/dragndrop.linux.cpp
	${CCL_DIR}/platform/linux/gui/exceptionhandler.cpp
	${CCL_DIR}/platform/linux/gui/exceptionhandler.h
	${CCL_DIR}/platform/linux/gui/fileselector.linux.cpp
	${CCL_DIR}/platform/linux/gui/fontresource.linux.cpp
	${CCL_DIR}/platform/linux/gui/gui.linux.cpp
	${CCL_DIR}/platform/linux/gui/keyevent.linux.cpp
	${CCL_DIR}/platform/linux/gui/mousecursor.linux.cpp
	${CCL_DIR}/platform/linux/gui/nativegraphics.linux.cpp
	${CCL_DIR}/platform/linux/gui/native3dsupport.linux.cpp
	${CCL_DIR}/platform/linux/gui/platformdialog.linux.cpp
	${CCL_DIR}/platform/linux/gui/printservice.linux.cpp
	${CCL_DIR}/platform/linux/gui/printservice.linux.h
	${CCL_DIR}/platform/linux/gui/systemshell.linux.cpp
	${CCL_DIR}/platform/linux/gui/textbox.linux.cpp
	${CCL_DIR}/platform/linux/gui/textbox.linux.h
	${CCL_DIR}/platform/linux/gui/tooltip.linux.cpp
	${CCL_DIR}/platform/linux/gui/transparentwindow.linux.cpp
	${CCL_DIR}/platform/linux/gui/transparentwindow.linux.h
	${CCL_DIR}/platform/linux/gui/webbrowserview.linux.cpp
	${CCL_DIR}/platform/linux/gui/window.linux.cpp
	${CCL_DIR}/platform/linux/gui/window.linux.h
	${CCL_DIR}/platform/linux/shared/xdgportalrequest.h
	${CCL_DIR}/platform/linux/interfaces/linuxiids.cpp
	${CCL_DIR}/platform/linux/skia/rasterrendertarget.cpp
	${CCL_DIR}/platform/linux/skia/rasterrendertarget.h
	${CCL_DIR}/platform/linux/skia/skiaengine.linux.cpp
	${CCL_DIR}/platform/linux/skia/skiaengine.linux.h
	${CCL_DIR}/platform/linux/skia/skialayer.linux.cpp
	${CCL_DIR}/platform/linux/skia/skialayer.linux.h
	${CCL_DIR}/platform/linux/skia/skiarendertarget.linux.cpp
	${CCL_DIR}/platform/linux/skia/skiarendertarget.linux.h
	${CCL_DIR}/platform/linux/wayland/activationtoken.cpp
	${CCL_DIR}/platform/linux/wayland/activationtoken.h
	${CCL_DIR}/platform/linux/wayland/datadevicehelper.cpp
	${CCL_DIR}/platform/linux/wayland/datadevicehelper.h
	${CCL_DIR}/platform/linux/wayland/dmabufferhelper.cpp
	${CCL_DIR}/platform/linux/wayland/dmabufferhelper.h
	${CCL_DIR}/platform/linux/wayland/imagesurface.cpp
	${CCL_DIR}/platform/linux/wayland/imagesurface.h
	${CCL_DIR}/platform/linux/wayland/inputhandler.cpp
	${CCL_DIR}/platform/linux/wayland/inputhandler.h
	${CCL_DIR}/platform/linux/wayland/monitorhelper.cpp
	${CCL_DIR}/platform/linux/wayland/monitorhelper.h
	${CCL_DIR}/platform/linux/wayland/subsurface.h
	${CCL_DIR}/platform/linux/wayland/surface.cpp
	${CCL_DIR}/platform/linux/wayland/surface.h
	${CCL_DIR}/platform/linux/wayland/waylandbuffer.cpp
	${CCL_DIR}/platform/linux/wayland/waylandbuffer.h
	${CCL_DIR}/platform/linux/wayland/waylandchildwindow.cpp
	${CCL_DIR}/platform/linux/wayland/waylandchildwindow.h
	${CCL_DIR}/platform/linux/wayland/waylandclient.cpp
	${CCL_DIR}/platform/linux/wayland/waylandclient.h
	${CCL_DIR}/platform/linux/wayland/waylandcompositor.cpp
	${CCL_DIR}/platform/linux/wayland/waylandcompositor.h
	${CCL_DIR}/platform/linux/wayland/waylandrendertarget.cpp
	${CCL_DIR}/platform/linux/wayland/waylandrendertarget.h
)

if (CCL_ENABLE_VULKAN)
	ccl_list_append_once (cclgui_linux_sources
		${CCL_DIR}/platform/linux/vulkan/vulkanclient.linux.cpp
		${CCL_DIR}/platform/linux/vulkan/vulkanrendertarget.linux.cpp
		${CCL_DIR}/platform/linux/vulkan/vulkanrendertarget.linux.h
	)
endif ()

if (CCL_ENABLE_OPENGLES2)
	ccl_list_append_once (cclgui_linux_sources
		${CCL_DIR}/platform/linux/opengles/openglesclient.linux.cpp
		${CCL_DIR}/platform/linux/opengles/openglesrendertarget.linux.cpp
		${CCL_DIR}/platform/linux/opengles/openglesrendertarget.linux.h
	)
endif ()

ccl_list_append_once (cclgui_platform_sources
	${CCL_DIR}/platform/shared/host/frameworkalert.cpp
	${CCL_DIR}/platform/shared/host/frameworkwindowdecoration.cpp
	${CCL_DIR}/platform/shared/host/platformalertbase.cpp
	${CCL_DIR}/platform/shared/host/platformfileselectorbase.cpp
	${CCL_DIR}/platform/shared/host/platformidletask.cpp
	${CCL_DIR}/platform/shared/host/platformintegration.cpp
	${CCL_DIR}/platform/shared/host/platformnotifyicon.cpp
	${CCL_DIR}/platform/shared/host/platformthemepainter.cpp
	${CCL_DIR}/platform/shared/host/platformthemepainter.h
	${CCL_DIR}/platform/shared/posix/gui/exceptionhandler.posix.cpp
	${CCL_DIR}/platform/shared/posix/gui/exceptionhandler.posix.h
	${CCL_DIR}/platform/shared/skia/skiabitmap.cpp
	${CCL_DIR}/platform/shared/skia/skiabitmap.h
	${CCL_DIR}/platform/shared/skia/skiadevice.cpp
	${CCL_DIR}/platform/shared/skia/skiadevice.h
	${CCL_DIR}/platform/shared/skia/skiaengine.cpp
	${CCL_DIR}/platform/shared/skia/skiaengine.h
	${CCL_DIR}/platform/shared/skia/skiafonttable.cpp
	${CCL_DIR}/platform/shared/skia/skiafonttable.h
	${CCL_DIR}/platform/shared/skia/skiaglue.h
	${CCL_DIR}/platform/shared/skia/skiagradient.cpp
	${CCL_DIR}/platform/shared/skia/skiagradient.h
	${CCL_DIR}/platform/shared/skia/skiapath.cpp
	${CCL_DIR}/platform/shared/skia/skiapath.h
	${CCL_DIR}/platform/shared/skia/skiarendertarget.cpp
	${CCL_DIR}/platform/shared/skia/skiarendertarget.h
	${CCL_DIR}/platform/shared/skia/skiastream.cpp
	${CCL_DIR}/platform/shared/skia/skiastream.h
	${CCL_DIR}/platform/shared/skia/skiatextlayout.cpp
	${CCL_DIR}/platform/shared/skia/skiatextlayout.h

	${CCL_DIR}/platform/shared/skia/test/skiatest.cpp
)

if (CCL_ENABLE_VULKAN)
	ccl_list_append_once (cclgui_platform_sources
		${CCL_DIR}/platform/shared/vulkan/vulkanclient.cpp
		${CCL_DIR}/platform/shared/vulkan/vulkanclient.h
		${CCL_DIR}/platform/shared/vulkan/vulkanimage.cpp
		${CCL_DIR}/platform/shared/vulkan/vulkanimage.h
		${CCL_DIR}/platform/shared/vulkan/vulkanrendertarget.cpp
		${CCL_DIR}/platform/shared/vulkan/vulkanrendertarget.h
		${CCL_DIR}/platform/shared/vulkan/vulkan3dsupport.cpp
		${CCL_DIR}/platform/shared/vulkan/vulkan3dsupport.h
		${CCL_DIR}/platform/shared/vulkan/vulkanskia3dsupport.cpp
		${CCL_DIR}/platform/shared/vulkan/vulkanskia3dsupport.h
		${CCL_DIR}/platform/shared/vulkan/vulkanskiarendertarget.cpp
		${CCL_DIR}/platform/shared/vulkan/vulkanskiarendertarget.h
	)
endif ()

if (CCL_ENABLE_OPENGLES2)
	ccl_list_append_once (cclgui_platform_sources
		${CCL_DIR}/platform/shared/opengles/openglesclient.cpp
		${CCL_DIR}/platform/shared/opengles/openglesclient.h
		${CCL_DIR}/platform/shared/opengles/openglesimage.cpp
		${CCL_DIR}/platform/shared/opengles/openglesimage.h
		${CCL_DIR}/platform/shared/opengles/openglesrendertarget.cpp
		${CCL_DIR}/platform/shared/opengles/openglesrendertarget.h
		${CCL_DIR}/platform/shared/opengles/opengles3dsupport.cpp
		${CCL_DIR}/platform/shared/opengles/opengles3dsupport.h
	)
endif ()

if (CCL_ENABLE_VULKAN OR CCL_ENABLE_OPENGLES2)
	ccl_list_append_once (cclgui_platform_sources
		${CCL_DIR}/platform/shared/opengles/glslshaderreflection.cpp
		${CCL_DIR}/platform/shared/opengles/glslshaderreflection.h
	)
endif ()
	
ccl_list_append_once (cclgui_sources
	${cclgui_linux_sources}
	${cclgui_platform_sources}
)

source_group (TREE ${CCL_DIR}/platform/linux PREFIX "source" FILES ${cclgui_linux_sources})
source_group (TREE ${CCL_DIR}/platform/shared PREFIX "source" FILES ${cclgui_platform_sources})

ccl_list_append_once (WAYLAND_PROTOCOLS
	"staging/xdg-activation/xdg-activation-v1.xml"
	"staging/xdg-dialog/xdg-dialog-v1.xml"
	"unstable/xdg-output/xdg-output-unstable-v1.xml"
	"unstable/xdg-decoration/xdg-decoration-unstable-v1.xml"
	"unstable/xdg-foreign/xdg-foreign-unstable-v1.xml"
	"unstable/xdg-foreign/xdg-foreign-unstable-v2.xml"
	"unstable/text-input/text-input-unstable-v3.xml"
	"unstable/pointer-constraints/pointer-constraints-unstable-v1.xml"
	"unstable/relative-pointer/relative-pointer-unstable-v1.xml"
)

find_package (wayland-server-delegate REQUIRED)
find_package (Wayland REQUIRED COMPONENTS ${wayland_components})
find_package (XKB REQUIRED)
find_package (sdbus-c++ REQUIRED)
find_package (Fontconfig REQUIRED)

find_package (skia REQUIRED COMPONENTS ${skia_components})
if (CMAKE_CROSSCOMPILING AND NOT FREETYPE_DIR)
	set (FREETYPE_DIR "${CMAKE_SYSROOT}/usr/include/freetype2")
endif ()

# Skia dependencies
set (skia_dependencies "")
if (SKIA_USE_SYSTEM_HARFBUZZ)
	find_library (HarfBuzz_LIBRARY NAMES harfbuzz)
	find_library (HarfBuzz_ICU_LIBRARY NAMES harfbuzz-icu)
	find_library (HarfBuzz_subset_LIBRARY NAMES harfbuzz-subset)
	set (HarfBuzz_LIBRARIES ${HarfBuzz_LIBRARY} ${HarfBuzz_ICU_LIBRARY})
	if (HarfBuzz_subset_LIBRARY)
		ccl_list_append_once (HarfBuzz_LIBRARIES ${HarfBuzz_subset_LIBRARY})
	endif ()
	list (APPEND skia_dependencies ${HarfBuzz_LIBRARIES})
	mark_as_advanced (HarfBuzz_LIBRARY HarfBuzz_ICU_LIBRARY HarfBuzz_subset_LIBRARY)
endif ()
if (SKIA_USE_SYSTEM_ICU)
	set (ICU_FIND_QUIETLY ON)
	find_package (ICU REQUIRED COMPONENTS data uc)
	list (APPEND skia_dependencies ${ICU_LIBRARIES})
endif ()
if (SKIA_USE_SYSTEM_FREETYPE)
	find_package (Freetype REQUIRED)
	list (APPEND skia_dependencies ${FREETYPE_LIBRARIES})
endif ()
if (SKIA_USE_SYSTEM_JPEG)
	find_package (JPEG REQUIRED)
	list (APPEND skia_dependencies ${JPEG_LIBRARIES})
endif ()

if (NOT ${CCL_STATIC_ONLY})
	target_link_libraries (${cclgui} PRIVATE m wayland-server-delegate ${WAYLAND_LIBRARIES} ${XKB_LIBRARIES} ${cclgui_graphics_libraries} SDBusCpp::sdbus-c++ ${Fontconfig_LIBRARIES} ${SKIA_LIBRARIES} ${skia_dependencies})
	target_include_directories (${cclgui} PRIVATE ${XKB_INCLUDE_PATH} ${FREETYPE_INCLUDE_DIRS} ${Fontconfig_INCLUDE_DIRS} ${cclgui_graphics_includes})
	target_compile_definitions (${cclgui} PRIVATE ${cclgui_graphics_definitions} SK_FONTCONFIG=1)
endif ()
ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES wayland-server-delegate ${WAYLAND_LIBRARIES} ${XKB_LIBRARIES} ${cclgui_graphics_libraries} SDBusCpp::sdbus-c++ ${Fontconfig_LIBRARIES} ${SKIA_LIBRARIES} ${skia_dependencies})
ccl_list_append_once (CCL_STATIC_COMPILE_DEFINITIONS ${cclgui_graphics_definitions} SK_FONTCONFIG=1)
ccl_list_append_once (CCL_STATIC_INCLUDE_DIRS ${XKB_INCLUDE_PATH} ${FREETYPE_INCLUDE_DIRS} ${Fontconfig_INCLUDE_DIRS} ${cclgui_graphics_includes})

ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/vertexshaderPN.vert PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/vertexshaderPNT.vert PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/vertexshaderPNT2PN.vert PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/billboardvertexshader.vert PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/solidcolormaterial.frag PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/texturematerial.frag PATH shaders)

include ("${CMAKE_CURRENT_LIST_DIR}/../platform/linux/cclgui-xdgportal/xdgportal.cmake")

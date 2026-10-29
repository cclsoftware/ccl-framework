
ccl_list_append_once (cclgui_wasm_sources
	${CCL_DIR}/platform/wasm/gui/alert.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/clipboard.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/desktop.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/dragndrop.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/exceptionhandler.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/exceptionhandler.wasm.h
	${CCL_DIR}/platform/wasm/gui/fileselector.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/fontresource.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/gui.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/inputmethod.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/keyevent.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/menu.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/mousecursor.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/native3dsupport.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/nativegraphics.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/printservice.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/systemevent.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/systemevent.wasm.h
	${CCL_DIR}/platform/wasm/gui/systemshell.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/textbox.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/theme.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/tooltip.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/transparentwindow.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/webbrowserview.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/window.wasm.cpp
	${CCL_DIR}/platform/wasm/gui/window.wasm.h
	${CCL_DIR}/platform/wasm/opengles/openglesrendertarget.wasm.cpp
	${CCL_DIR}/platform/wasm/skia/skiaengine.wasm.cpp
	${CCL_DIR}/platform/wasm/skia/skiaengine.wasm.h
	${CCL_DIR}/platform/wasm/skia/skialayer.wasm.cpp
	${CCL_DIR}/platform/wasm/skia/skialayer.wasm.h
	${CCL_DIR}/platform/wasm/skia/skiarendertarget.wasm.cpp
)

ccl_list_append_once (cclgui_platform_sources
	${CCL_DIR}/platform/shared/host/frameworkalert.cpp
	${CCL_DIR}/platform/shared/host/frameworkalert.h

	${CCL_DIR}/platform/shared/opengles/openglesimage.cpp
	${CCL_DIR}/platform/shared/opengles/openglesimage.h
	${CCL_DIR}/platform/shared/opengles/openglesrendertarget.cpp
	${CCL_DIR}/platform/shared/opengles/openglesrendertarget.h
	${CCL_DIR}/platform/shared/opengles/opengles3dsupport.cpp
	${CCL_DIR}/platform/shared/opengles/opengles3dsupport.h
	${CCL_DIR}/platform/shared/opengles/glslshaderreflection.cpp
	${CCL_DIR}/platform/shared/opengles/glslshaderreflection.h

	${CCL_DIR}/platform/shared/skia/skiabitmap.cpp
	${CCL_DIR}/platform/shared/skia/skiabitmap.h
	${CCL_DIR}/platform/shared/skia/skiadevice.cpp
	${CCL_DIR}/platform/shared/skia/skiadevice.h
	${CCL_DIR}/platform/shared/skia/skiaengine.cpp
	${CCL_DIR}/platform/shared/skia/skiaengine.h
	${CCL_DIR}/platform/shared/skia/skiafontmanager.cpp
	${CCL_DIR}/platform/shared/skia/skiafontmanager.h
	${CCL_DIR}/platform/shared/skia/skiafontresource.cpp
	${CCL_DIR}/platform/shared/skia/skiafonttable.cpp
	${CCL_DIR}/platform/shared/skia/skiafonttable.h
	${CCL_DIR}/platform/shared/skia/skiaglue.h
	${CCL_DIR}/platform/shared/skia/skiagradient.cpp
	${CCL_DIR}/platform/shared/skia/skiagradient.h
	${CCL_DIR}/platform/shared/skia/skialayer.cpp
	${CCL_DIR}/platform/shared/skia/skialayer.h
	${CCL_DIR}/platform/shared/skia/skiapath.cpp
	${CCL_DIR}/platform/shared/skia/skiapath.h
	${CCL_DIR}/platform/shared/skia/skiaprintservice.cpp
	${CCL_DIR}/platform/shared/skia/skiaprintservice.h
	${CCL_DIR}/platform/shared/skia/skiarendertarget.cpp
	${CCL_DIR}/platform/shared/skia/skiarendertarget.h
	${CCL_DIR}/platform/shared/skia/skiastream.cpp
	${CCL_DIR}/platform/shared/skia/skiastream.h
	${CCL_DIR}/platform/shared/skia/skiatextlayout.cpp
	${CCL_DIR}/platform/shared/skia/skiatextlayout.h

	${CCL_DIR}/platform/shared/skia/test/skiatest.cpp
)

ccl_list_append_once (cclgui_sources
	${cclgui_wasm_sources}
	${cclgui_platform_sources}
)

source_group (TREE ${CCL_DIR}/platform/wasm PREFIX "source" FILES ${cclgui_wasm_sources})
source_group (TREE ${CCL_DIR}/platform/shared PREFIX "source" FILES ${cclgui_platform_sources})

find_package (skia REQUIRED COMPONENTS webgl)
find_package (OpenGL REQUIRED COMPONENTS GL)

ccl_list_append_once (cclgui_graphics_libraries OpenGL::GL)
ccl_list_append_once (cclgui_graphics_includes ${OPENGL_INCLUDE_DIR})
ccl_list_append_once (cclgui_graphics_definitions SK_GANESH=1 SK_GL=1 SK_ASSUME_WEBGL=1)

ccl_check_imported (${cclgui} imported)
if (NOT imported)
	if (NOT ${CCL_STATIC_ONLY})
		target_include_directories (${cclgui} PRIVATE ${yoga_DIR})			
		target_include_directories (${cclgui} PRIVATE ${cclgui_graphics_includes} ${skia_SOURCE_DIR})
		target_compile_definitions (${cclgui} PRIVATE ${cclgui_graphics_definitions})
		target_compile_options (${cclgui} PRIVATE -fno-exceptions)
		target_link_libraries (${cclgui} PRIVATE ${cclgui_graphics_libraries} ${SKIA_LIBRARIES})
	endif ()
	ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${SKIA_LIBRARIES} ${cclgui_graphics_libraries})
	ccl_list_append_once (CCL_STATIC_INCLUDE_DIRS ${cclgui_graphics_includes})
	ccl_list_append_once (CCL_STATIC_COMPILE_DEFINITIONS ${cclgui_graphics_definitions})

	set_source_files_properties (${CCL_DIR}/platform/wasm/gui/systemevent.wasm.cpp PROPERTIES COMPILE_FLAGS "-frtti") # required for embind

	ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/vertexshaderPN.vert PATH shaders)
	ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/vertexshaderPNT.vert PATH shaders)
	ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/vertexshaderPNT2PN.vert PATH shaders)
	ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/billboardvertexshader.vert PATH shaders)
	ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/solidcolormaterial.frag PATH shaders)
	ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/texturematerial.frag PATH shaders)
endif ()

target_link_options (${cclgui} INTERFACE 
	"--preload-file=${CCL_DIR}/packaging/wasm/resource/fonts/OpenSans-Regular.ttf@/fonts/OpenSans-Regular.ttf"
	"--preload-file=${CCL_DIR}/packaging/wasm/resource/fonts/SourceCodePro-Regular.ttf@/fonts/SourceCodePro-Regular.ttf"
	"--preload-file=${CCL_DIR}/meta/generated/javascript/gui-constants-generated.js@/scripts/gui-constants.js"
)
if (VENDOR_STYLE_SHEET)
	set (cclStyleSheet "${VENDOR_STYLE_SHEET}")
else ()
	ccl_find_file (cclStyleSheet NAMES ccl.css HINTS "${CCL_DIR}/packaging/wasm/resource/")
endif ()
if (cclStyleSheet)
	target_link_options (${cclgui} INTERFACE "--preload-file=${cclStyleSheet}@/styles/ccl.css")
endif ()
string (APPEND VENDOR_EMSCRIPTEN_LINKER_FLAGS " --pre-js=${CCL_DIR}/platform/wasm/js/cclgui.js")

download_webfonts ()

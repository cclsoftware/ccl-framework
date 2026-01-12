find_package (webview2 REQUIRED)

ccl_list_append_once (${cclgui}_icons
    1	${CCL_DIR}/packaging/win/resource/cclapp.ico
)

ccl_list_append_once (cclgui_platform_graphics_sources
	${CCL_DIR}/platform/win/direct2d/d2dbase.cpp
	${CCL_DIR}/platform/win/direct2d/d2dbase.h
	${CCL_DIR}/platform/win/direct2d/d2dbitmap.cpp
	${CCL_DIR}/platform/win/direct2d/d2dbitmap.h
	${CCL_DIR}/platform/win/direct2d/d2dclipper.cpp
	${CCL_DIR}/platform/win/direct2d/d2dclipper.h
	${CCL_DIR}/platform/win/direct2d/d2ddevice.cpp
	${CCL_DIR}/platform/win/direct2d/d2ddevice.h
	${CCL_DIR}/platform/win/direct2d/d2dengine.cpp
	${CCL_DIR}/platform/win/direct2d/d2dengine.h
	${CCL_DIR}/platform/win/direct2d/d2dgradient.cpp
	${CCL_DIR}/platform/win/direct2d/d2dgradient.h
	${CCL_DIR}/platform/win/direct2d/d2dinterop.h
	${CCL_DIR}/platform/win/direct2d/d2dpath.cpp
	${CCL_DIR}/platform/win/direct2d/d2dpath.h
	${CCL_DIR}/platform/win/direct2d/d2dprintjob.cpp
	${CCL_DIR}/platform/win/direct2d/d2dprintjob.h
	${CCL_DIR}/platform/win/direct2d/d2dtextlayout.cpp
	${CCL_DIR}/platform/win/direct2d/d2dtextlayout.h
	${CCL_DIR}/platform/win/direct2d/d2dwindow.cpp
	${CCL_DIR}/platform/win/direct2d/d2dwindow.h
	${CCL_DIR}/platform/win/direct2d/d3dsupport.cpp
	${CCL_DIR}/platform/win/direct2d/d3dsupport.h
	${CCL_DIR}/platform/win/direct2d/dcompengine.cpp
	${CCL_DIR}/platform/win/direct2d/dcompengine.h
	${CCL_DIR}/platform/win/direct2d/dwriteengine.cpp
	${CCL_DIR}/platform/win/direct2d/dwriteengine.h
	${CCL_DIR}/platform/win/direct2d/dxgiengine.cpp
	${CCL_DIR}/platform/win/direct2d/dxgiengine.h
	${CCL_DIR}/platform/win/direct2d/wicbitmaphandler.cpp
	${CCL_DIR}/platform/win/direct2d/wicbitmaphandler.h
)

ccl_list_append_once (cclgui_platform_graphics_source_files
    ${CCL_DIR}/platform/win/gui/nativegraphics.win.cpp
    ${CCL_DIR}/platform/win/gui/printservice.win.cpp
    ${CCL_DIR}/platform/win/gui/printservice.win.h
)

ccl_list_append_once (cclgui_platform_windows_sources
    ${CCL_DIR}/platform/win/gui/desktop.win.cpp
    ${CCL_DIR}/platform/win/gui/dialog.win.cpp
    ${CCL_DIR}/platform/win/gui/tooltip.win.cpp
    ${CCL_DIR}/platform/win/gui/layeredwindowrendertarget.cpp
    ${CCL_DIR}/platform/win/gui/layeredwindowrendertarget.h
    ${CCL_DIR}/platform/win/gui/transparentwindow.win.cpp
    ${CCL_DIR}/platform/win/gui/transparentwindow.win.h
    ${CCL_DIR}/platform/win/gui/window.win.cpp
    ${CCL_DIR}/platform/win/gui/window.win.h
)

ccl_list_append_once (cclgui_platform_system_sources
    ${CCL_DIR}/platform/win/gui/clipboard.win.cpp
    ${CCL_DIR}/platform/win/gui/dragndrop.win.cpp
    ${CCL_DIR}/platform/win/gui/dragndrop.win.h
    ${CCL_DIR}/platform/win/gui/fontresource.win.cpp
    ${CCL_DIR}/platform/win/gui/notifyicon.win.cpp
    ${CCL_DIR}/platform/win/gui/systemshell.win.cpp
    ${CCL_DIR}/platform/win/gui/webbrowserview.win.cpp
    ${CCL_DIR}/platform/win/gui/webbrowserview2.win.cpp
)

ccl_list_append_once (cclgui_platform_dialogs_sources
    ${CCL_DIR}/platform/win/gui/alert.win.cpp
    ${CCL_DIR}/platform/win/gui/fileselector.win.cpp
)

ccl_list_append_once (cclgui_platform_controls_sources
    ${CCL_DIR}/platform/win/gui/textbox.win.cpp
    ${CCL_DIR}/platform/win/gui/textbox.win.h
)

ccl_list_append_once (cclgui_platform_popup_sources
    ${CCL_DIR}/platform/win/gui/menu.win.cpp
    ${CCL_DIR}/platform/win/gui/menu.win.h
)

ccl_list_append_once (cclgui_platform_theme_sources
    ${CCL_DIR}/platform/win/gui/theme.win.cpp
    ${CCL_DIR}/platform/win/gui/theme.win.h
)

ccl_list_append_once (cclgui_platform_win32_sources
    ${CCL_DIR}/platform/win/gui/accessibility.win.cpp
    ${CCL_DIR}/platform/win/gui/accessibility.win.h
    ${CCL_DIR}/platform/win/gui/activex.cpp
    ${CCL_DIR}/platform/win/gui/activex.h
    ${CCL_DIR}/platform/win/gui/dpihelper.cpp
    ${CCL_DIR}/platform/win/gui/dpihelper.h
    ${CCL_DIR}/platform/win/gui/exceptionhandler.cpp
    ${CCL_DIR}/platform/win/gui/exceptionhandler.h
    ${CCL_DIR}/platform/win/gui/mousecursor.win.cpp
    ${CCL_DIR}/platform/win/gui/oledragndrop.cpp
    ${CCL_DIR}/platform/win/gui/oledragndrop.h
    ${CCL_DIR}/platform/win/gui/screenscaling.cpp
    ${CCL_DIR}/platform/win/gui/screenscaling.h
    ${CCL_DIR}/platform/win/gui/shellhelper.cpp
    ${CCL_DIR}/platform/win/gui/shellhelper.h
    ${CCL_DIR}/platform/win/gui/taskbar.cpp
    ${CCL_DIR}/platform/win/gui/taskbar.h
    ${CCL_DIR}/platform/win/gui/touchhelper.cpp
    ${CCL_DIR}/platform/win/gui/touchhelper.h
    ${CCL_DIR}/platform/win/gui/windowhelper.cpp
    ${CCL_DIR}/platform/win/gui/windowhelper.h
    ${CCL_DIR}/platform/win/gui/windowclasses.h
    ${CCL_DIR}/platform/win/gui/win32graphics.cpp
    ${CCL_DIR}/platform/win/gui/win32graphics.h
    ${CCL_DIR}/platform/win/system/cclcom.cpp
    ${CCL_DIR}/platform/win/system/cclcom.h
    ${CCL_DIR}/platform/win/system/comstream.cpp
    ${CCL_DIR}/platform/win/system/comstream.h
    ${CCL_DIR}/platform/win/system/registry.cpp
    ${CCL_DIR}/platform/win/system/registry.h
)

ccl_list_append_once (cclgui_platform_sources
    ${CCL_DIR}/platform/win/gui/gui.win.cpp
    ${CCL_DIR}/platform/win/gui/keyevent.win.cpp
)

ccl_list_append_once (cclgui_sources
    ${cclgui_platform_graphics_sources}
    ${cclgui_platform_graphics_source_files}
    ${cclgui_platform_windows_sources}
    ${cclgui_platform_system_sources}
    ${cclgui_platform_dialogs_sources}
    ${cclgui_platform_controls_sources}
    ${cclgui_platform_popup_sources}
    ${cclgui_platform_theme_sources}
    ${cclgui_platform_win32_sources}
    ${cclgui_platform_sources}
	${CMAKE_CURRENT_LIST_DIR}/visualstudio/cclgui.natvis
)	

source_group (TREE ${CCL_DIR}/platform/win PREFIX "source/graphics" FILES ${cclgui_platform_graphics_sources})
source_group ("source/graphics" FILES ${cclgui_platform_graphics_source_files})
source_group ("source/windows" FILES ${cclgui_platform_windows_sources})
source_group ("source/system" FILES ${cclgui_platform_system_sources})
source_group ("source/dialogs" FILES ${cclgui_platform_dialogs_sources})
source_group ("source/controls" FILES ${cclgui_platform_controls_sources})
source_group ("source/popup" FILES ${cclgui_platform_popup_sources})
source_group ("source/theme" FILES ${cclgui_platform_theme_sources})
source_group ("source/system/win32" FILES ${cclgui_platform_win32_sources})
source_group ("source" FILES ${cclgui_platform_sources})
source_group ("" FILES ${CMAKE_CURRENT_LIST_DIR}/visualstudio/ccltext.natvis)

if (NOT ${CCL_STATIC_ONLY})
    ccl_embed_manifest (${cclgui} "${CMAKE_CURRENT_LIST_DIR}/../packaging/win/cclgui.manifest")
endif ()

ccl_add_vertexshader (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/d3d/vertexshaderPN.hlsl PATH shaders)
ccl_add_vertexshader (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/d3d/vertexshaderPNT.hlsl PATH shaders)
ccl_add_vertexshader (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/d3d/vertexshaderPNT2PN.hlsl PATH shaders)
ccl_add_vertexshader (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/d3d/billboardvertexshader.hlsl PATH shaders)
ccl_add_pixelshader (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/d3d/solidcolormaterial.hlsl PATH shaders)
ccl_add_pixelshader (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/d3d/texturematerial.hlsl PATH shaders)

target_link_options (${cclgui} PRIVATE "$<$<CONFIG:DEBUG>:/NODEFAULTLIB:\"MSVCRT\">")
target_link_libraries (${cclgui} PUBLIC $<BUILD_INTERFACE:webview2>)  

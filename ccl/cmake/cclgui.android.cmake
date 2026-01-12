include_guard (DIRECTORY)

find_package (vulkan-utility-libraries REQUIRED)

set_source_files_properties (
	${CCL_DIR}/gui/views/view.cpp
	PROPERTIES COMPILE_FLAGS "-fexceptions"
)

ccl_list_append_once (cclgui_android_sources
	${CCL_DIR}/platform/android/cclandroidjni.h

	${CCL_DIR}/platform/android/graphics/android3dsupport.cpp
	${CCL_DIR}/platform/android/graphics/android3dsupport.h
	${CCL_DIR}/platform/android/graphics/androidbitmap.cpp
	${CCL_DIR}/platform/android/graphics/androidbitmap.h
	${CCL_DIR}/platform/android/graphics/androidfont.cpp
	${CCL_DIR}/platform/android/graphics/androidfont.h
	${CCL_DIR}/platform/android/graphics/androidgradient.cpp
	${CCL_DIR}/platform/android/graphics/androidgradient.h
	${CCL_DIR}/platform/android/graphics/androidgraphics.cpp
	${CCL_DIR}/platform/android/graphics/androidgraphics.h
	${CCL_DIR}/platform/android/graphics/androidpath.cpp
	${CCL_DIR}/platform/android/graphics/androidpath.h
	${CCL_DIR}/platform/android/graphics/androidrendertarget.cpp
	${CCL_DIR}/platform/android/graphics/androidrendertarget.h
	${CCL_DIR}/platform/android/graphics/androidtextlayout.cpp
	${CCL_DIR}/platform/android/graphics/androidtextlayout.h
	${CCL_DIR}/platform/android/graphics/frameworkgraphics.cpp
	${CCL_DIR}/platform/android/graphics/frameworkgraphics.h
	${CCL_DIR}/platform/android/graphics/paintcache.cpp
	${CCL_DIR}/platform/android/graphics/paintcache.h
	${CCL_DIR}/platform/android/graphics/ttfparser.h

	${CCL_DIR}/platform/android/gui/accessibility.android.cpp
	${CCL_DIR}/platform/android/gui/accessibility.android.h
	${CCL_DIR}/platform/android/gui/alert.android.cpp
	${CCL_DIR}/platform/android/gui/androidintent.cpp
	${CCL_DIR}/platform/android/gui/androidintent.h
	${CCL_DIR}/platform/android/gui/androidview.cpp
	${CCL_DIR}/platform/android/gui/androidview.h
	${CCL_DIR}/platform/android/gui/clipboard.android.cpp
	${CCL_DIR}/platform/android/gui/desktop.android.cpp
	${CCL_DIR}/platform/android/gui/dragndrop.android.cpp
	${CCL_DIR}/platform/android/gui/fileselector.android.cpp
	${CCL_DIR}/platform/android/gui/fontresource.android.cpp
	${CCL_DIR}/platform/android/gui/frameworkactivity.cpp
	${CCL_DIR}/platform/android/gui/frameworkactivity.h
	${CCL_DIR}/platform/android/gui/frameworkview.cpp
	${CCL_DIR}/platform/android/gui/frameworkview.h
	${CCL_DIR}/platform/android/gui/graphicslayer.android.cpp
	${CCL_DIR}/platform/android/gui/graphicslayer.android.h
	${CCL_DIR}/platform/android/gui/gui.android.cpp
	${CCL_DIR}/platform/android/gui/keyevent.android.cpp
	${CCL_DIR}/platform/android/gui/keyevent.android.h
	${CCL_DIR}/platform/android/gui/menu.android.cpp
	${CCL_DIR}/platform/android/gui/nativegraphics.android.cpp
	${CCL_DIR}/platform/android/gui/printservice.android.cpp
	${CCL_DIR}/platform/android/gui/systemsharing.android.cpp
	${CCL_DIR}/platform/android/gui/systemshell.android.cpp
	${CCL_DIR}/platform/android/gui/textbox.android.cpp
	${CCL_DIR}/platform/android/gui/theme.android.cpp
	${CCL_DIR}/platform/android/gui/transparentwindow.android.cpp
	${CCL_DIR}/platform/android/gui/webbrowserview.android.cpp
	${CCL_DIR}/platform/android/gui/window.android.cpp
	${CCL_DIR}/platform/android/gui/window.android.h

	${CCL_DIR}/platform/android/interfaces/androidnativeiids.cpp
	${CCL_DIR}/platform/android/interfaces/iandroidsystem.h
	${CCL_DIR}/platform/android/interfaces/iframeworkactivity.h

	${CCL_DIR}/platform/android/interfaces/jni/androidcontent.cpp
	${CCL_DIR}/platform/android/interfaces/jni/androidcontent.h

	${CCL_DIR}/platform/android/vulkan/vulkanclient.android.cpp
	${CCL_DIR}/platform/android/vulkan/vulkanrendertarget.android.cpp
	${CCL_DIR}/platform/android/vulkan/vulkanrendertarget.android.h
	${CCL_DIR}/platform/android/vulkan/vulkansurfaceview.cpp
	${CCL_DIR}/platform/android/vulkan/vulkansurfaceview.h
)

ccl_list_append_once (cclgui_android_java_sources
	${CCL_DIR}/platform/android/graphics/java/FrameworkGraphics.java
	${CCL_DIR}/platform/android/graphics/java/FrameworkGraphicsFactory.java
	${CCL_DIR}/platform/android/graphics/java/FrameworkGraphicsPath.java
	${CCL_DIR}/platform/android/graphics/java/FrameworkTextLayout.java

	${CCL_DIR}/platform/android/gui/java/Alert.java
	${CCL_DIR}/platform/android/gui/java/FrameworkActivity.java
	${CCL_DIR}/platform/android/gui/java/FrameworkDialog.java
	${CCL_DIR}/platform/android/gui/java/FrameworkView.java
	${CCL_DIR}/platform/android/gui/java/GraphicsLayerView.java
	${CCL_DIR}/platform/android/gui/java/PrintPageRenderer.java
	${CCL_DIR}/platform/android/gui/java/SystemTimer.java
	${CCL_DIR}/platform/android/gui/java/TextControl.java
	${CCL_DIR}/platform/android/gui/java/WebControl.java

	${CCL_DIR}/platform/android/vulkan/java/VulkanSurfaceView.java
)

ccl_list_append_once (cclgui_android_additional_sources
	${CCL_DIR}/gui/graphics/imaging/codecs/pngcodec.cpp
	${CCL_DIR}/gui/graphics/imaging/codecs/pngcodec.h
)

ccl_list_append_once (cclgui_platform_sources
	${CCL_DIR}/platform/shared/opengles/glslshaderreflection.cpp
	${CCL_DIR}/platform/shared/opengles/glslshaderreflection.h
	${CCL_DIR}/platform/shared/vulkan/vulkanclient.cpp
	${CCL_DIR}/platform/shared/vulkan/vulkanclient.h
	${CCL_DIR}/platform/shared/vulkan/vulkanimage.cpp
	${CCL_DIR}/platform/shared/vulkan/vulkanimage.h
	${CCL_DIR}/platform/shared/vulkan/vulkanrendertarget.cpp
	${CCL_DIR}/platform/shared/vulkan/vulkanrendertarget.h
	${CCL_DIR}/platform/shared/vulkan/vulkan3dsupport.cpp
	${CCL_DIR}/platform/shared/vulkan/vulkan3dsupport.h
)

ccl_list_append_once (cclgui_sources
	${cclgui_android_sources}
	${cclgui_android_java_sources}
	${cclgui_android_additional_sources}
	${cclgui_platform_sources}

	${corelib_android_jnionload_sources}
)

source_group ("core" FILES ${corelib_android_jnionload_sources})
source_group (TREE ${CCL_DIR}/platform/android PREFIX "source/platform" FILES ${cclgui_android_sources})
source_group (TREE ${CCL_DIR}/platform/android PREFIX "source/platform" FILES ${cclgui_android_java_sources})
source_group (TREE ${CCL_DIR}/platform/shared PREFIX "source/platform" FILES ${cclgui_platform_sources})
source_group (TREE ${CCL_DIR}/gui PREFIX "source" FILES ${cclgui_android_additional_sources})

find_library (JNIGraphics_LIBRARY NAMES jnigraphics)
find_library (Vulkan_LIBRARY NAMES vulkan)

if (NOT ${CCL_STATIC_ONLY})
	target_link_libraries (${cclgui} PRIVATE ${JNIGraphics_LIBRARY} ${Vulkan_LIBRARY} ${Vulkan_Utility_Libraries_LIBRARY})
endif ()

ccl_list_append_once (CCL_STATIC_LINK_LIBRARIES ${JNIGraphics_LIBRARY} ${Vulkan_LIBRARY} ${Vulkan_Utility_Libraries_LIBRARY})

ccl_add_aar_project (${cclgui} NAMESPACE "dev.ccl.cclgui" DEPENDS corelib cclsystem)
ccl_install_aar (${cclgui} COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})

ccl_add_gradle_dependency (${cclgui} "androidx.documentfile:documentfile:1.0.1" TRANSITIVE)

ccl_add_java_sourcedir (${cclgui} "${CCL_DIR}/meta/generated/java")
ccl_add_java_sourcedir (${cclgui} "${CCL_DIR}/platform/android/graphics/java")
ccl_add_java_sourcedir (${cclgui} "${CCL_DIR}/platform/android/gui/java")
ccl_add_java_sourcedir (${cclgui} "${CCL_DIR}/platform/android/vulkan/java")

ccl_add_gradle_include (${cclgui} "${CCL_DIR}/packaging/android/cclgui.gradle")

ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/vertexshaderPN.vert PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/vertexshaderPNT.vert PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/vertexshaderPNT2PN.vert PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/billboardvertexshader.vert PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/solidcolormaterial.frag PATH shaders)
ccl_add_shader_resource (${cclgui} ${CCL_DIR}/gui/graphics/3d/shader/glsl/texturematerial.frag PATH shaders)

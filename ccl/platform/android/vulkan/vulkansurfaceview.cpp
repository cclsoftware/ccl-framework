//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/android/vulkan/vulkansurfaceview.cpp
// Description : Vulkan surface view JNI interface
//
//************************************************************************************************

#include "ccl/platform/android/vulkan/vulkansurfaceview.h"
#include "ccl/platform/android/vulkan/vulkanrendertarget.android.h"

#include "ccl/platform/android/gui/androidview.h"
#include "ccl/platform/android/gui/frameworkactivity.h"
#include "ccl/platform/android/gui/frameworkview.h"
#include "ccl/platform/android/gui/window.android.h"

#include "ccl/base/message.h"

#include "ccl/gui/gui.h"

#include <android/native_window_jni.h>

//************************************************************************************************
// JNI classes
//************************************************************************************************

namespace CCL {
namespace Android {

//************************************************************************************************
// RenderSurfaceView
//************************************************************************************************

DECLARE_JNI_CLASS (RenderSurfaceView, CCLGUI_CLASS_PREFIX "VulkanSurfaceView")
	DECLARE_JNI_CONSTRUCTOR (construct, jobject)
	DECLARE_JNI_METHOD (void, layout, int, int, int, int)
	DECLARE_JNI_METHOD (void, startRendering)
	DECLARE_JNI_METHOD (void, stopRendering)
	DECLARE_JNI_METHOD (bool, isAlive)
END_DECLARE_JNI_CLASS (RenderSurfaceView)

DEFINE_JNI_CLASS (RenderSurfaceView)
	DEFINE_JNI_CONSTRUCTOR (construct, "(JLandroid/content/Context;)V")
	DEFINE_JNI_METHOD (layout, "(IIII)V")
	DEFINE_JNI_METHOD (startRendering, "()V")
	DEFINE_JNI_METHOD (stopRendering, "()V")
	DEFINE_JNI_METHOD (isAlive, "()Z")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// VulkanSurfaceView
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (VulkanSurfaceView, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

VulkanSurfaceView::VulkanSurfaceView (Window* parent, VulkanWindowRenderTarget* renderTarget)
: parent (parent),
  renderTarget (renderTarget),
  nativeWindow (nullptr)
{
	JniAccessor jni;
	androidView.assign (jni, jni.newObject (RenderSurfaceView, RenderSurfaceView.construct, asIntPtr (), FrameworkActivity::getCurrentActivity ()->getJObject ()));

	setSize (parent->getSize ());

	// add surface view to the window's content view
	if(FrameworkView* frameworkView = AndroidWindow::cast (parent)->getFrameworkView ())
		ViewGroup.addView (*frameworkView, androidView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VulkanSurfaceView::~VulkanSurfaceView ()
{
	if(!androidView)
		return;

	cancelSignals ();

	JniAccessor jni;

	// remove view
	if(FrameworkView* frameworkView = AndroidWindow::cast (parent)->getFrameworkView ())
		ViewGroup.removeView (*frameworkView, androidView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanSurfaceView::setSize (RectRef size)
{
	Rect zoomedSize = Rect (size).zoom (renderTarget->getScaleFactor ());

	JniAccessor jni;
	RenderSurfaceView.layout (androidView, zoomedSize.left, zoomedSize.top, zoomedSize.right, zoomedSize.bottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanSurfaceView::startRendering ()
{
	JniAccessor jni;
	RenderSurfaceView.startRendering (androidView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanSurfaceView::stopRendering ()
{
	JniAccessor jni;
	RenderSurfaceView.stopRendering (androidView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanSurfaceView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "render")
		onRender ();
	else if(msg == "surfaceCreated")
		onSurfaceCreated (msg[0]);
	else if(msg == "surfaceDestroyed")
		onSurfaceDestroyed ();
	else if(msg == "surfaceResized")
		onSurfaceResized (msg[0], msg[1]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanSurfaceView::onSurfaceCreated (VariantRef surface)
{
	JniAccessor jni;
	if(!RenderSurfaceView.isAlive (androidView))
		return;

	nativeWindow = (ANativeWindow*) surface.asIntPointer ();

	renderTarget->onSurfaceCreated (nativeWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanSurfaceView::onSurfaceDestroyed ()
{
	if(!nativeWindow)
		return;

	renderTarget->onSurfaceDestroyed ();

	ANativeWindow_release (nativeWindow);

	nativeWindow = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanSurfaceView::onSurfaceResized (int width, int height)
{
	if(!nativeWindow)
		return;

	renderTarget->onSurfaceResized (width, height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanSurfaceView::onRender ()
{
	if(!nativeWindow)
		return;

	renderTarget->onRender ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, VulkanSurfaceView, onSurfaceCreatedNative, JniIntPtr nativeViewPtr, jobject surface)
{
	VulkanSurfaceView* nativeView = VulkanSurfaceView::fromIntPtr (nativeViewPtr);
	ASSERT (nativeView)
	if(!nativeView)
		return;

	JniAccessor jni;
	ANativeWindow* nativeWindow = ANativeWindow_fromSurface (jni, surface);

	(NEW Message ("surfaceCreated", UIntPtr (nativeWindow)))->postBlocking (nativeView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, VulkanSurfaceView, onSurfaceDestroyedNative, JniIntPtr nativeViewPtr)
{
	VulkanSurfaceView* nativeView = VulkanSurfaceView::fromIntPtr (nativeViewPtr);
	ASSERT (nativeView)
	if(!nativeView)
		return;

	(NEW Message ("surfaceDestroyed"))->postBlocking (nativeView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, VulkanSurfaceView, onSurfaceResizedNative, JniIntPtr nativeViewPtr, int width, int height)
{
	VulkanSurfaceView* nativeView = VulkanSurfaceView::fromIntPtr (nativeViewPtr);
	ASSERT (nativeView)
	if(!nativeView)
		return;

	(NEW Message ("surfaceResized", width, height))->post (nativeView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, VulkanSurfaceView, onRenderNative, JniIntPtr nativeViewPtr)
{
	VulkanSurfaceView* nativeView = VulkanSurfaceView::fromIntPtr (nativeViewPtr);
	ASSERT (nativeView)
	if(!nativeView)
		return;

	// Post the render message only when the application is active as not returning to the Java
	// side in an inactive application blocks the main thread so the message is never processed.
	if(GUI.isApplicationActive ())
		(NEW Message ("render"))->postBlocking (nativeView);
}

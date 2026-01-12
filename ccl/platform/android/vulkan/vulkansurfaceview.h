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
// Filename    : ccl/platform/android/vulkan/vulkansurfaceview.h
// Description : Vulkan surface view JNI interface
//
//************************************************************************************************

#ifndef _ccl_vulkansurfaceview_h
#define _ccl_vulkansurfaceview_h

#include "ccl/gui/windows/window.h"

#include "ccl/platform/android/cclandroidjni.h"

#include <android/native_window.h>

namespace CCL {
namespace Android {

class VulkanWindowRenderTarget;

//************************************************************************************************
// VulkanSurfaceView
//************************************************************************************************

class VulkanSurfaceView: public Object,
						 public JniCast<VulkanSurfaceView>
{
public:
	DECLARE_CLASS_ABSTRACT (VulkanSurfaceView, Object)

	VulkanSurfaceView (Window* parent, VulkanWindowRenderTarget* renderTarget);
	~VulkanSurfaceView ();

	void setSize (RectRef size);

	void startRendering ();
	void stopRendering ();

	void onSurfaceCreated (VariantRef surface);
	void onSurfaceDestroyed ();
	void onSurfaceResized (int width, int height);
	void onRender ();

	// Object
	void notify (ISubject* subject, MessageRef msg) override;

private:
	Window* parent;
	VulkanWindowRenderTarget* renderTarget;

	JniObject androidView;
	ANativeWindow* nativeWindow;
};

} // namespace Android
} // namespace CCL

#endif // _ccl_vulkansurfaceview_h

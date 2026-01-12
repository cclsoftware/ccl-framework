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
// Filename    : ccl/platform/android/vulkan/vulkanrendertarget.android.h
// Description : Android Render Target using Vulkan
//
//************************************************************************************************

#ifndef _vulkanrendertarget_android_h
#define _vulkanrendertarget_android_h

#include "ccl/platform/android/graphics/androidrendertarget.h"

#include "ccl/platform/android/vulkan/vulkansurfaceview.h"

#include "ccl/platform/shared/vulkan/vulkanrendertarget.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// AndroidVulkanRenderTarget
//************************************************************************************************

class AndroidVulkanRenderTarget: public VulkanRenderTarget
{
public:
	AndroidVulkanRenderTarget ();
	~AndroidVulkanRenderTarget ();

	static bool isSupported ();

	void onSurfaceCreated (ANativeWindow* surface);
	void onSurfaceDestroyed ();
	void onSurfaceResized (int width, int height);

protected:
	bool pauseRendering;

	Vector<VkCommandBuffer> commandBuffers;

	bool initialize (ANativeWindow* window);
	bool destroy ();

	void createCommandBuffers ();
	void freeCommandBuffers ();
};

//************************************************************************************************
// VulkanWindowRenderTarget
//************************************************************************************************

class VulkanWindowRenderTarget: public AndroidVulkanRenderTarget,
								public AndroidWindowRenderTarget
{
public:
	VulkanWindowRenderTarget (Window& window);

	// AndroidVulkanRenderTarget
	float getOpacity () const override;
	bool isTranslucent () const override;
	float getScaleFactor () const override;
	
	// AndroidWindowRenderTarget
	void onSize () override;
	void onRender () override;
	void onScroll (RectRef rect, PointRef delta) override;
	IMutableRegion* getUpdateRegion () override { return nullptr; }
	IMutableRegion* getInvalidateRegion () override { return &invalidateRegion; }
	bool shouldCollectUpdates () override { return true; }
	void add3DSurface (Native3DSurface* surface) override;
	void remove3DSurface (Native3DSurface* surface) override;
	
private:
	AutoPtr<VulkanSurfaceView> surfaceView;
	MutableRegion invalidateRegion;
		
	// AndroidVulkanRenderTarget
	bool initialize () override;
	void clear () override;
};

} // namespace Android
} // namespace CCL

#endif // _vulkanrendertarget_android_h

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
// Filename    : ccl/platform/shared/vulkan/vulkanrendertarget.h
// Description : Skia Render Target using Vulkan
//
//************************************************************************************************

#ifndef _vulkanrendertarget_h
#define _vulkanrendertarget_h

#include "ccl/platform/shared/vulkan/vulkan3dsupport.h"
#include "ccl/platform/shared/vulkan/vulkanimage.h"

namespace CCL {

//************************************************************************************************
// VulkanRenderTarget
//************************************************************************************************

class VulkanRenderTarget
{
public:
	VulkanRenderTarget ();
	virtual ~VulkanRenderTarget ();
	
	virtual float getScaleFactor () const { return 1.f; }
	virtual float getOpacity () const { return 1.f; }
	virtual bool isTranslucent () const { return false; }
	
	PROPERTY_AUTO_POINTER (Vulkan3DGraphicsContext, graphicsContext3D, GraphicsContext3D)
	
protected:
	enum class InitializeLevel
	{
		kDevice,
		kSwapchain,
		k3DSurfaces
	};

	VkSurfaceKHR vulkanSurface;
	VkSurfaceFormatKHR format;
	VkSwapchainKHR swapChain;
	Vector<VkImage> swapChainImages;
	VkExtent2D extent;
	VkExtent2D swapChainExtent;
	VkQueue presentationQueue;

	Vector<Vulkan3DSurface*> surfaces;

	Vector<VkSemaphore> imageAvailableSemaphores;
	Vector<VkSemaphore> renderFinishedSemaphores;
	Vector<VkSemaphore> renderFinishedSemaphores3D;
	Vector<VkSemaphore> compositionSemaphores;
	
	Vector<VkRectLayerKHR> presentRegion;
	bool incrementalUpdateEnabled;
	bool imageSamplingEnabled;
	
	VkSemaphore signalSemaphore[2]; // signals completion for 2D drawing (Skia), triggers presentation
	VkSemaphore lastSignalSemaphore; // signalSemaphore from previous frame
	VkSemaphore waitSemaphore; // signals image availability, 2D drawing (Skia) waits for this signal (3D is rendered offscreen, so there is no need to wait for a swapchain image)
	
	int maxFramesInFlight;
	int currentFrame;
	uint32_t currentImage;
	bool sizeChanged;
	bool outOfDate;

	virtual bool initialize () { return true; }
	virtual bool initialize3DSurfaces ();
	virtual void clear () {}
	
	virtual void reinitialize (InitializeLevel level = InitializeLevel::kDevice);

	bool initializeQueues ();
	bool initializeSwapChain ();
	void destroySwapChain (VkSwapchainKHR swapChain);
	void destroySemaphores ();
	void destroy3DSurfaces ();
	VkImage nextImage ();
	void nextFrame ();
	void render3DContent ();

	virtual bool flushSurface ();

	void presentFrame ();
	void addVulkan3DSurface (Native3DSurface* surface);
	void removeVulkan3DSurface (Native3DSurface* surface);
};

} // namespace CCL

#endif // _vulkanrendertarget_h

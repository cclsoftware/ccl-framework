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
// Filename    : ccl/platform/android/vulkan/vulkanrendertarget.android.cpp
// Description : Android Render Target using Vulkan
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/android/vulkan/vulkanrendertarget.android.h"

#include "ccl/platform/android/gui/window.android.h"

#include "ccl/platform/android/graphics/android3dsupport.h"

#include <vulkan/vulkan_android.h>

using namespace CCL;
using namespace Android;

//************************************************************************************************
// AndroidVulkanRenderTarget
//************************************************************************************************

AndroidVulkanRenderTarget::AndroidVulkanRenderTarget ()
: pauseRendering (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidVulkanRenderTarget::~AndroidVulkanRenderTarget ()
{
	freeCommandBuffers ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidVulkanRenderTarget::initialize (ANativeWindow* window)
{
	if(!window)
		return false;
	
	VulkanClient& vulkanClient = VulkanClient::instance ();
	
	// create Vulkan Android surface
	VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.window = window;
	VkResult result = vkCreateAndroidSurfaceKHR (vulkanClient.getVulkanInstance (), &surfaceCreateInfo, nullptr, &vulkanSurface);
	ASSERT (result == VK_SUCCESS && vulkanSurface != VK_NULL_HANDLE)
	if(vulkanSurface == VK_NULL_HANDLE)
		return false;

	if(!vulkanClient.isInitialized ())
	{
		vulkanClient.initialize (vulkanSurface);
		ASSERT (vulkanClient.isInitialized ())
		if(!vulkanClient.isInitialized ())
			return false;
	}
	else
	{
		VkBool32 presentationSupport = false;
		VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR (vulkanClient.getPhysicalDevice (), vulkanClient.getPresentationQueueFamilyIndex (), vulkanSurface, &presentationSupport);
		ASSERT (result == VK_SUCCESS && presentationSupport)
	}
	
	const auto& deviceExtensions = vulkanClient.getDeviceExtensions ();
	if(deviceExtensions.contains (VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME))
		incrementalUpdateEnabled = true;
	
	initializeQueues ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidVulkanRenderTarget::destroy ()
{
	if(vulkanSurface == VK_NULL_HANDLE)
		return false;

	VulkanClient& vulkanClient = VulkanClient::instance ();
	vkDestroySurfaceKHR (vulkanClient.getVulkanInstance (), vulkanSurface, nullptr);
	vulkanSurface = VK_NULL_HANDLE;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidVulkanRenderTarget::onSurfaceCreated (ANativeWindow* surface)
{
	bool initialized = initialize (surface);
	ASSERT (initialized)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidVulkanRenderTarget::onSurfaceDestroyed ()
{
	freeCommandBuffers ();

	destroySwapChain (swapChain);
	swapChain = VK_NULL_HANDLE;

	destroy ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidVulkanRenderTarget::onSurfaceResized (int width, int height)
{
	extent.width = width;
	extent.height = height;

	freeCommandBuffers ();

	// (re-)initialize swapchain
	bool initialized = initializeSwapChain ();
	if(initialized)
		createCommandBuffers ();

	ASSERT (initialized)

	lastSignalSemaphore = VK_NULL_HANDLE;
	for(Vulkan3DSurface* surface : surfaces)
		surface->invalidate ();

	clear ();

	pauseRendering = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidVulkanRenderTarget::createCommandBuffers ()
{
	VulkanClient& vulkanClient = VulkanClient::instance ();
	VkDevice device = vulkanClient.getLogicalDevice ();

	// create command buffers for each swap chain image
	commandBuffers.setCount (swapChainImages.count ());

	VkCommandBufferAllocateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	createInfo.commandPool = vulkanClient.getCommandPool ();
	createInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	createInfo.commandBufferCount = uint32 (commandBuffers.count ());

	vkAllocateCommandBuffers (device, &createInfo, commandBuffers);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidVulkanRenderTarget::freeCommandBuffers ()
{
	if(commandBuffers.isEmpty ())
		return;

	VulkanClient& vulkanClient = VulkanClient::instance ();
	VkDevice device = vulkanClient.getLogicalDevice ();

	vkFreeCommandBuffers (device, vulkanClient.getCommandPool (), commandBuffers.count (), commandBuffers);
	commandBuffers.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidVulkanRenderTarget::isSupported ()
{
	VulkanClient& vulkanClient = VulkanClient::instance ();
	if(!vulkanClient.isInitialized ())
		vulkanClient.initialize (VK_NULL_HANDLE);

	ASSERT (vulkanClient.isInitialized ())
	return vulkanClient.isInitialized ();
}

//************************************************************************************************
// VulkanWindowRenderTarget
//************************************************************************************************

VulkanWindowRenderTarget::VulkanWindowRenderTarget (Window& window)
: AndroidWindowRenderTarget (window)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanWindowRenderTarget::initialize ()
{
	surfaceView = NEW VulkanSurfaceView (&window, this);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::clear ()
{
	invalidateRegion.setEmpty ();
	invalidateRegion.addRect (Rect (0, 0, swapChainExtent.width, swapChainExtent.height));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::onSize ()
{
	if(surfaceView == nullptr)
		return;

	surfaceView->setSize (window.getSize ());
	pauseRendering = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float VulkanWindowRenderTarget::getOpacity () const
{
	return window.getOpacity ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanWindowRenderTarget::isTranslucent () const
{
	return window.getStyle ().isTranslucent () || getOpacity () < 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float VulkanWindowRenderTarget::getScaleFactor () const
{
	return window.getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::onRender ()
{
	if(pauseRendering)
		return;

	if(!invalidateRegion.getRects ().isEmpty ())
	{
		VkImage image = nextImage ();
		if(image == VK_NULL_HANDLE)
			return;

		render3DContent ();

		// blend prerendered 3D surfaces to swap chain image
		VkCommandBufferBeginInfo beginInfo {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer (commandBuffers[currentImage], &beginInfo);

		VkClearColorValue transparentColor {};
		VkImageSubresourceRange range { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdClearColorImage (commandBuffers[currentImage], swapChainImages[currentImage], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &transparentColor, 1, &range);

		for(Vulkan3DSurface* surface : surfaces)
		{
			if(!surface->isValid ())
				continue;

			Rect surfaceRect (surface->getSize ());
			surfaceRect.zoom (getScaleFactor ());

			uint32 width = uint32 (surfaceRect.getWidth ());
			uint32 height = uint32 (surfaceRect.getHeight ());

			VkImageCopy copyInfo {};
			copyInfo.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyInfo.srcOffset = { 0, 0, 0 };
			copyInfo.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyInfo.dstOffset = { surfaceRect.left, surfaceRect.top, 0 };
			copyInfo.extent = { width, height, 1 };

			VkImage image = static_cast<Android3DSurface*> (surface)->getVulkanImage ();
			vkCmdCopyImage (commandBuffers[currentImage], image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[currentImage], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);

			presentRegion.add ({ VkOffset2D {surfaceRect.left, surfaceRect.top}, VkExtent2D {width, height}, 0 });
		}

		vkEndCommandBuffer (commandBuffers[currentImage]);

		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
		submitInfo.pWaitDstStageMask = &waitStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentImage];

		vkQueueSubmit (presentationQueue, 1, &submitInfo, VK_NULL_HANDLE);

		presentFrame ();
		nextFrame ();
		
		invalidateRegion.setEmpty ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::onScroll (RectRef rect, PointRef delta)
{
	invalidateRegion.addRect (Rect (0, 0, window.getWidth (), window.getHeight ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::add3DSurface (Native3DSurface* surface)
{
	if(surfaceView == nullptr)
		initialize ();

	addVulkan3DSurface (surface);

	if(surfaces.count () == 1)
		surfaceView->startRendering ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::remove3DSurface (Native3DSurface* surface)
{
	if(surfaces.count () == 1)
		surfaceView->stopRendering ();

	removeVulkan3DSurface (surface);
}

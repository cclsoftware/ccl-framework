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
// Filename    : ccl/platform/shared/vulkan/vulkanrendertarget.cpp
// Description : Skia Render Target using Vulkan
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/vulkan/vulkanrendertarget.h"

#include <vulkan/vk_enum_string_helper.h>

using namespace CCL;

//************************************************************************************************
// VulkanRenderTarget
//************************************************************************************************

VulkanRenderTarget::VulkanRenderTarget ()
: vulkanSurface (VK_NULL_HANDLE),
  presentationQueue (VK_NULL_HANDLE),
  swapChain (VK_NULL_HANDLE),
  waitSemaphore (VK_NULL_HANDLE),
  signalSemaphore { VK_NULL_HANDLE, VK_NULL_HANDLE },
  lastSignalSemaphore (VK_NULL_HANDLE),
  maxFramesInFlight (0),
  currentFrame (0),
  currentImage (0),
  sizeChanged (false),
  outOfDate (false),
  extent {0, 0},
  swapChainExtent {0, 0},
  incrementalUpdateEnabled (false),
  imageSamplingEnabled (false),
  format {VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

VulkanRenderTarget::~VulkanRenderTarget ()
{
	VulkanClient& vulkanClient = VulkanClient::instance ();
	
	destroy3DSurfaces ();
	destroySemaphores ();
	
	if(vulkanSurface != VK_NULL_HANDLE)
	{
		if(swapChain != VK_NULL_HANDLE)
			destroySwapChain (swapChain);
		vkDestroySurfaceKHR (vulkanClient.getVulkanInstance (), vulkanSurface, nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderTarget::reinitialize (InitializeLevel level)
{
	VulkanClient& vulkanClient = VulkanClient::instance ();

	if(level <= InitializeLevel::k3DSurfaces)
	{
		destroy3DSurfaces ();
		if(level <= InitializeLevel::kSwapchain)
		{
			destroySemaphores ();
			destroySwapChain (swapChain);
			swapChain = VK_NULL_HANDLE;
			if(level <= InitializeLevel::kDevice)
			{
				vulkanClient.initializeLogicalDevice ();
				initializeQueues ();
			}
			initializeSwapChain ();
		}
		initialize3DSurfaces ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanRenderTarget::initializeQueues ()
{
	VulkanClient& vulkanClient = VulkanClient::instance ();
	
	presentationQueue = vulkanClient.getPresentationQueue ();
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanRenderTarget::initializeSwapChain ()
{
	ASSERT (vulkanSurface != VK_NULL_HANDLE)
	
	VulkanClient& vulkanClient = VulkanClient::instance ();
	
	VkSwapchainKHR oldSwapChain = swapChain;
	
	// check physical device formats
	
	Vector<VkSurfaceFormatKHR> formats;
	format.format = VK_FORMAT_UNDEFINED;
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR (vulkanClient.getPhysicalDevice (), vulkanSurface, &formatCount, nullptr);
	formats.setCount (formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR (vulkanClient.getPhysicalDevice (), vulkanSurface, &formatCount, formats);
	for(const VkSurfaceFormatKHR& availableFormat : formats)
	{
		format = availableFormat;
		if(availableFormat.format == VulkanImage::kNativeImageFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			break;
	}
	
	if(format.format == VK_FORMAT_UNDEFINED)
	{
		CCL_WARN ("%s\n", "No matching image format available.")
		return false;
	}
	
	// check device capabilities
	
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR (vulkanClient.getPhysicalDevice (), vulkanSurface, &capabilities);

	swapChainExtent.width = ccl_max (capabilities.minImageExtent.width, ccl_min (capabilities.maxImageExtent.width, extent.width));
	swapChainExtent.height = ccl_max (capabilities.minImageExtent.height, ccl_min (capabilities.maxImageExtent.height, extent.height));
	
	uint32_t imageCount = capabilities.minImageCount + 1;
	if(capabilities.maxImageCount > 0)
		imageCount = ccl_min (imageCount, capabilities.maxImageCount);
	
	VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT 
								| VK_IMAGE_USAGE_TRANSFER_DST_BIT 
								| VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
								| VK_IMAGE_USAGE_SAMPLED_BIT;
	if((imageUsage ^ (imageUsage & capabilities.supportedUsageFlags)) != 0)
		CCL_WARN ("Device does not support requested image usage flags (want 0x%x, have 0x%x).\n", imageUsage, imageUsage & capabilities.supportedUsageFlags)
	imageUsage &= capabilities.supportedUsageFlags;
	
	imageSamplingEnabled = get_flag<VkFlags> (capabilities.supportedUsageFlags, VK_IMAGE_USAGE_SAMPLED_BIT);
	
	// check supported present modes
		
	Vector<VkPresentModeKHR> presentModes;
	VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR (vulkanClient.getPhysicalDevice (), vulkanSurface, &presentModeCount, nullptr);
	presentModes.setCount (presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR (vulkanClient.getPhysicalDevice (), vulkanSurface, &presentModeCount, presentModes);
	
	if(presentModes.contains (VK_PRESENT_MODE_MAILBOX_KHR))
		selectedPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	else if(presentModes.contains (VK_PRESENT_MODE_FIFO_KHR))
	{
		CCL_WARN ("%s\n", "Mailbox mode not supported. Falling back to FIFO.")
		selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	}
	else if(!presentModes.isEmpty ())
	{
		CCL_WARN ("Neither Mailbox nor FIFO mode are supported. Falling back to mode %d.\n", presentModes[0])
		selectedPresentMode = presentModes[0];
	}
	else
	{
		CCL_WARN ("%s\n", "GPU driver does not report any present modes. Trying FIFO mode anyway.\n")
	}

	// create the swapchain
	
	VkSwapchainCreateInfoKHR createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = vulkanSurface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageExtent = swapChainExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = imageUsage;
	createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	
	uint32_t queueFamilyIndices[] = { vulkanClient.getGraphicsQueueFamilyIndex (), vulkanClient.getPresentationQueueFamilyIndex () };

	createInfo.pQueueFamilyIndices = queueFamilyIndices;
	
	if(queueFamilyIndices[0] != queueFamilyIndices[1])
	{
		createInfo.queueFamilyIndexCount = 2;
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else 
	{
		createInfo.queueFamilyIndexCount = 1;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	
	if(get_flag<VkFlags> (capabilities.supportedCompositeAlpha, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR))
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	else
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = selectedPresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapChain;
	
	VkDevice device = vulkanClient.getLogicalDevice ();
	VkResult result = vkCreateSwapchainKHR (device, &createInfo, nullptr, &swapChain);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS || swapChain == VK_NULL_HANDLE)
	{
		CCL_WARN ("%s %s\n", "Failed to create a swap chain!", string_VkResult (result))
		return false;
	}
	
	imageCount = 0;
	vkGetSwapchainImagesKHR (device, swapChain, &imageCount, nullptr);
	swapChainImages.setCount (imageCount);
	result = vkGetSwapchainImagesKHR (device, swapChain, &imageCount, swapChainImages);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s %s\n", "Could not get swap chain images from device.", string_VkResult (result))
		return false;
	}
	
	// Create semaphores
	
	maxFramesInFlight = imageCount * 2;
	CCL_PRINTF ("max frames in flight: %d\n", maxFramesInFlight)
	
	if(imageAvailableSemaphores.count () != maxFramesInFlight)
	{		
		destroySemaphores ();
		
		imageAvailableSemaphores.setCount (maxFramesInFlight);
		renderFinishedSemaphores.setCount (maxFramesInFlight * 2);
		renderFinishedSemaphores3D.setCount (maxFramesInFlight);
		
		VkSemaphoreCreateInfo semaphoreCreateInfo {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		VkFenceCreateInfo fenceCreateInfo {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		
		for(int i = 0; i < maxFramesInFlight; i++)
		{
			result = vkCreateSemaphore (vulkanClient.getLogicalDevice (), &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]);
			ASSERT (result == VK_SUCCESS)
			result = vkCreateSemaphore (vulkanClient.getLogicalDevice (), &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i * 2]);
			ASSERT (result == VK_SUCCESS)
			result = vkCreateSemaphore (vulkanClient.getLogicalDevice (), &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i * 2 + 1]);
			ASSERT (result == VK_SUCCESS)
			result = vkCreateSemaphore (vulkanClient.getLogicalDevice (), &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores3D[i]);
			ASSERT (result == VK_SUCCESS)
		}
		
		currentFrame = currentFrame % maxFramesInFlight;
	}
	
	if(oldSwapChain != VK_NULL_HANDLE)
		destroySwapChain (oldSwapChain);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderTarget::destroySwapChain (VkSwapchainKHR swapChain)
{
	VulkanClient& vulkanClient = VulkanClient::instance ();
	
	vkDeviceWaitIdle (vulkanClient.getLogicalDevice ());
	vkDestroySwapchainKHR (vulkanClient.getLogicalDevice (), swapChain, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderTarget::destroySemaphores ()
{
	VulkanClient& vulkanClient = VulkanClient::instance ();
	VkDevice device = vulkanClient.getLogicalDevice ();
	
	if(device)
		vkDeviceWaitIdle (device);

	signalSemaphore[0] = VK_NULL_HANDLE;
	signalSemaphore[1] = VK_NULL_HANDLE;
	lastSignalSemaphore = VK_NULL_HANDLE;
	waitSemaphore = VK_NULL_HANDLE;
	
	for(int i = 0; i < imageAvailableSemaphores.count (); i++)
		vkDestroySemaphore (device, imageAvailableSemaphores[i], nullptr);
	for(int i = 0; i < renderFinishedSemaphores.count (); i++)
		vkDestroySemaphore (device, renderFinishedSemaphores[i], nullptr);
	for(int i = 0; i < renderFinishedSemaphores3D.count (); i++)
		vkDestroySemaphore (device, renderFinishedSemaphores3D[i], nullptr);
	
	imageAvailableSemaphores.setCount (0);
	renderFinishedSemaphores.setCount (0);
	renderFinishedSemaphores3D.setCount (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VkImage VulkanRenderTarget::nextImage ()
{
	VulkanClient& vulkanClient = VulkanClient::instance ();

    VkResult result = vkAcquireNextImageKHR (vulkanClient.getLogicalDevice (), swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &currentImage);
	
	if(result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		outOfDate = true;
		reinitialize (InitializeLevel::kSwapchain);
		return VK_NULL_HANDLE;
	}
	ASSERT (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)

	ASSERT (currentImage < swapChainImages.count ())
	if(currentImage >= swapChainImages.count ())
		return VK_NULL_HANDLE;

	waitSemaphore = imageAvailableSemaphores[currentFrame];
	signalSemaphore[0] = renderFinishedSemaphores[currentImage * 2];
	signalSemaphore[1] = renderFinishedSemaphores[currentImage * 2 + 1];

	return swapChainImages[currentImage];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderTarget::nextFrame ()
{
	currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanRenderTarget::flushSurface ()
{
	outOfDate = false;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderTarget::presentFrame ()
{
	VkPresentInfoKHR presentInfo {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &signalSemaphore[0];
	
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &currentImage;
	
	presentInfo.pResults = nullptr;
	
	VkPresentRegionsKHR presentRegionInfo {};
	VkPresentRegionKHR presentRegionData {};
	if(incrementalUpdateEnabled && !presentRegion.isEmpty ())
	{
		presentInfo.pNext = &presentRegionInfo;
		presentRegionInfo.sType = VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
		presentRegionInfo.swapchainCount = 1;
		presentRegionInfo.pRegions = &presentRegionData;
		presentRegionData.pRectangles = presentRegion;
		presentRegionData.rectangleCount = presentRegion.count ();
	}
	
	VkResult result = vkQueuePresentKHR (presentationQueue, &presentInfo);
	ASSERT (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
	
	presentRegion.empty ();

	sizeChanged = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderTarget::render3DContent ()
{
	compositionSemaphores.removeAll ();

	if(graphicsContext3D == nullptr)
		return;

	for(Vulkan3DSurface* surface : surfaces)
	{
		if(!surface->isValid () && !surface->create (VulkanClient::instance ().getGPUContext (), format.format, getScaleFactor (), swapChainImages.count ()))
			continue;
		
		VkSemaphore semaphore = surface->render (*graphicsContext3D);
		if(semaphore)
			compositionSemaphores.add (semaphore);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanRenderTarget::initialize3DSurfaces ()
{
	for(Vulkan3DSurface* surface : surfaces)
		surface->create (VulkanClient::instance ().getGPUContext (), format.format, getScaleFactor (), swapChainImages.count ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderTarget::destroy3DSurfaces ()
{
	for(Vulkan3DSurface* surface : surfaces)
		surface->destroy ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderTarget::addVulkan3DSurface (Native3DSurface* surface)
{
	if(Vulkan3DSurface* vulkan3DSurface = ccl_cast<Vulkan3DSurface> (surface))
	{
		if(swapChainImages.count () > 0)
			vulkan3DSurface->create (VulkanClient::instance ().getGPUContext (), format.format, getScaleFactor (), swapChainImages.count ());
		surfaces.add (vulkan3DSurface);
	}

	if(graphicsContext3D == nullptr)
		graphicsContext3D = NEW Vulkan3DGraphicsContext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderTarget::removeVulkan3DSurface (Native3DSurface* surface)
{
	for(int i = 0; i < surfaces.count (); i++)
	{
		if(surfaces[i] == ccl_cast<Vulkan3DSurface> (surface))
		{
			surfaces[i]->destroy ();
			surfaces.removeAt (i);
			break;
		}
	}
}

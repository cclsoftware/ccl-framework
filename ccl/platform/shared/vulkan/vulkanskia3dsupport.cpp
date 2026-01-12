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
// Filename    : ccl/platform/shared/vulkan/vulkanskia3dsupport.cpp
// Description : Vulkan Skia 3D Support
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/vulkan/vulkanskia3dsupport.h"
#include "ccl/platform/shared/skia/skiabitmap.h"

using namespace CCL;

//************************************************************************************************
// SkiaVulkan3DSurface
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkiaVulkan3DSurface, Vulkan3DSurface)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaVulkan3DSurface::create (VulkanGPUContext* gpuContext, VkFormat format, float scaleFactor, int count)
{
	bool result = Vulkan3DSurface::create (gpuContext, format, scaleFactor, count);
	ASSERT (result)
	if(!result)
		return false;

	VkExtent2D extent {};
	extent.width = viewPortRect.getWidth ();
	extent.height = viewPortRect.getHeight ();

	// create Skia images wrapping the resolve images
	skiaImages.setCount (count);
	for(int i = 0; i < count; i++)
	{
		GrVkImageInfo imageInfo;
		imageInfo.fImage = resolveImages[i].getImage ();
		imageInfo.fImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.fImageTiling = resolveImages[i].getTiling ();
		imageInfo.fImageUsageFlags = resolveImages[i].getUsage ();
		imageInfo.fFormat = resolveImages[i].getFormat ();
		imageInfo.fLevelCount = 1;
		imageInfo.fCurrentQueueFamily = VK_QUEUE_FAMILY_IGNORED;
		imageInfo.fSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		GrBackendTexture texture = GrBackendTextures::MakeVk (extent.width, extent.height, imageInfo);
		skiaImages[i] = SkImages::BorrowTextureFrom (gpuContext, texture, kTopLeft_GrSurfaceOrigin, kBGRA_8888_SkColorType, kPremul_SkAlphaType, nullptr);
		if(skiaImages[i] == nullptr)
		{
			CCL_WARN ("%s\n", "Failed to wrap a resolve image for presentation.")
			destroy ();
			return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaVulkan3DSurface::destroy ()
{
	SuperClass::destroy ();
	
	skiaImages.setCount (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaVulkan3DSurface::isValid () const
{
	return SuperClass::isValid () && !skiaImages.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkImage> SkiaVulkan3DSurface::getSkiaImage () const
{
	return currentCommandBuffer >= 0 ? skiaImages[currentCommandBuffer] : nullptr;	
}

//************************************************************************************************
// SkiaVulkanClient
//************************************************************************************************

bool SkiaVulkanClient::initializeLogicalDevice ()
{
	gpuContext = nullptr;

	return VulkanClient::initializeLogicalDevice ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaVulkanClient::terminate ()
{
	gpuContext = nullptr;

	VulkanClient::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VulkanGPUContext* SkiaVulkanClient::getGPUContext ()
{
	if(gpuContext == nullptr)
	{
		if(!initialized || graphicsQueue == nullptr || logicalDevice == nullptr)
			return nullptr;

		auto getProc = [] (CStringPtr procName, VkInstance instance, VkDevice device) -> PFN_vkVoidFunction 
		{
			PFN_vkVoidFunction proc = nullptr;
			if(device != nullptr)
				proc = vkGetDeviceProcAddr (device, procName);
			if(proc == nullptr)
				proc = vkGetInstanceProcAddr (instance, procName);
			if(proc == nullptr)
				CCL_WARN ("Could not find function address for %s with instance %p and device %p\n", procName, instance, device)
			return proc;
		};
	
		skgpu::VulkanExtensions extensions;
		extensions.init (getProc, vulkanInstance, physicalDevice, vulkanExtensions.count (), vulkanExtensions, deviceExtensions.count (), deviceExtensions);

		skgpu::VulkanBackendContext skiaBackendContext {};
		skiaBackendContext.fQueue = graphicsQueue;
		skiaBackendContext.fDevice = logicalDevice;
		skiaBackendContext.fInstance = vulkanInstance;
		skiaBackendContext.fPhysicalDevice = physicalDevice;
		skiaBackendContext.fDeviceFeatures = &physicalDeviceFeatures;
		skiaBackendContext.fGraphicsQueueIndex = graphicsQueueFamilyIndex;
		skiaBackendContext.fMaxAPIVersion = kAPIVersion;
		skiaBackendContext.fVkExtensions = &extensions;
		skiaBackendContext.fGetProc = getProc;
	
		gpuContext = GrDirectContexts::MakeVulkan (skiaBackendContext);
	}
	return gpuContext.get ();
}

//************************************************************************************************
// SkiaVulkan3DSupport
//************************************************************************************************

Native3DGraphicsFactory& SkiaVulkan3DSupport::get3DFactory ()
{
	return Vulkan3DSupport::instance ().get3DFactory ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DSurface* SkiaVulkan3DSupport::create3DSurface ()
{
	return NEW SkiaVulkan3DSurface;
}

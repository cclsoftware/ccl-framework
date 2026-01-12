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
// Filename    : ccl/platform/shared/vulkan/vulkanskiarendertarget.cpp
// Description : Vulkan Skia Render Target
//
//************************************************************************************************

#include "vulkanskiarendertarget.h"
#include "vulkanskia3dsupport.h"

using namespace CCL;

//************************************************************************************************
// SkiaVulkanRenderTarget
//************************************************************************************************

void SkiaVulkanRenderTarget::reinitialize (InitializeLevel level)
{
	VulkanRenderTarget::reinitialize (level);
	
	setSurface (nullptr);
	lastSurface = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaVulkanRenderTarget::flushSurface ()
{
	VulkanClient& vulkanClient = VulkanClient::instance ();

	// flush 2D graphics

	sk_sp<SkSurface> surface = getSurface ();
	ASSERT (surface != nullptr)
	
	GrBackendSemaphore semaphores[2];
	semaphores[0] = GrBackendSemaphores::MakeVk (signalSemaphore[0]);
	semaphores[1] = GrBackendSemaphores::MakeVk (signalSemaphore[1]);

	GrFlushInfo flushInfo {};
	flushInfo.fNumSemaphores = ARRAY_COUNT (semaphores);
	flushInfo.fSignalSemaphores = semaphores;
	
	skgpu::MutableTextureState state = skgpu::MutableTextureStates::MakeVulkan (VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, vulkanClient.getGraphicsQueueFamilyIndex ());
	
	if(surface->recordingContext ()->asDirectContext ()->flush (surface.get (), flushInfo, &state) != GrSemaphoresSubmitted::kYes)
		return false;

	// While resizing a surface (e.g. a window), Nvidia drivers return VK_ERROR_OUT_OF_DATE_KHR all the time.
	// This makes us reinitialize the swapchain on each frame, so nothing is ever rendered to the screen.
	// Synchronizing the GPU here seems to fix this. This makes resizing a little sluggish, but at least we render something...
	bool syncGpu = sizeChanged && outOfDate;

	if(surface->recordingContext ()->asDirectContext ()->submit (syncGpu ? GrSyncCpu::kYes : GrSyncCpu::kNo) == false)
	{
		if(surface->recordingContext ()->asDirectContext ()->abandoned ())
		{
			// We might have lost the logical device, try to recreate
			surface = nullptr;
			reinitialize (InitializeLevel::kDevice);
		}
		return false;
	}

	outOfDate = false;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* SkiaVulkanRenderTarget::getSkiaCanvas ()
{
	sk_sp<SkSurface> surface = getSurface ();
	if(surface == nullptr)
	{
		if(vulkanSurface == nullptr)
			initialize ();
	
		if(swapChain == nullptr || sizeChanged)
			reinitialize (InitializeLevel::kSwapchain);
		
		if(swapChain == nullptr)
			return nullptr;
		
		VkImage image = nextImage ();
		if(image == nullptr)
			return nullptr;

		render3DContent ();

		bool needsClear = false;
		bool needsScale = false;
		
		GrVkImageInfo imageInfo;
		imageInfo.fImage = image;
		imageInfo.fImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.fImageTiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.fFormat = format.format;
		imageInfo.fLevelCount = 1;
		imageInfo.fCurrentQueueFamily = VK_QUEUE_FAMILY_IGNORED;
		imageInfo.fSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		GrBackendTexture texture = GrBackendTextures::MakeVk (swapChainExtent.width, swapChainExtent.height, imageInfo);

		SkSurfaceProps props;
		
		if(lastSurface && (incrementalUpdateEnabled  || imageSamplingEnabled))
		{
			setSurface (lastSurface);
			surface = getSurface ();
			surface->replaceBackendTexture (texture, kTopLeft_GrSurfaceOrigin, imageSamplingEnabled ? SkSurface::kRetain_ContentChangeMode : SkSurface::kDiscard_ContentChangeMode);
		}
		else
		{
			setSurface (SkSurfaces::WrapBackendTexture (static_cast<GrRecordingContext*> (VulkanClient::instance ().getGPUContext ()), texture, kTopLeft_GrSurfaceOrigin, 1, kBGRA_8888_SkColorType, nullptr, &props));
			surface = getSurface ();
			needsClear = true;
			needsScale = true;
		}
		lastSurface = nullptr;
		
		ASSERT (surface != nullptr)
		if(surface == nullptr)
			return nullptr;
		
		Vector<GrBackendSemaphore> semaphores;
		int semaphoreCount = 1;
		if(lastSignalSemaphore)
			semaphoreCount++;
		semaphoreCount += compositionSemaphores.count ();
		semaphores.setCount (semaphoreCount);
		
		int indexOffset = 0;
		semaphores[indexOffset++] = GrBackendSemaphores::MakeVk (waitSemaphore);
		if(lastSignalSemaphore)
			semaphores[indexOffset++] = GrBackendSemaphores::MakeVk (lastSignalSemaphore);
		for(int i = 0; i < compositionSemaphores.count (); i++)
			semaphores[indexOffset + i] = GrBackendSemaphores::MakeVk (compositionSemaphores[i]);

		lastSignalSemaphore = nullptr;
		
		if(surface->wait (semaphores.count (), semaphores, false) == false)
		{
			ASSERT (false) // why would this fail?
			return nullptr;
		}

		SkCanvas* canvas = surface->getCanvas ();
		ASSERT (canvas != nullptr)
		if(canvas == nullptr)
			return nullptr;
		
		if(needsScale)
			canvas->scale (getScaleFactor (), getScaleFactor ());
		
		if(needsClear)
		{
			canvas->clear (SkColorSetARGB (0, 0, 0, 0));
			clear ();
		}
	}
	
	ASSERT (surface != nullptr)
	return surface ? surface->getCanvas () : nullptr;
}

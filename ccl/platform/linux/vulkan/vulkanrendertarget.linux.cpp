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
// Filename    : ccl/platform/linux/vulkan/vulkanrendertarget.linux.cpp
// Description : Skia Render Target using Vulkan and Wayland
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/vulkan/vulkanrendertarget.linux.h"
#include "ccl/platform/linux/gui/window.linux.h"

#include "ccl/platform/shared/vulkan/vulkanskia3dsupport.h"

#include <vulkan/vulkan_wayland.h>

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxVulkanRenderTarget
//************************************************************************************************

LinuxVulkanRenderTarget::~LinuxVulkanRenderTarget ()
{
	if(!WaylandClient::instance ().isInitialized ())
	{
		// we lost previously allocated compositor objects, trying to destroy those could freeze the application
		vulkanSurface = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxVulkanRenderTarget::initialize (wl_display* display, wl_surface* surface)
{
	setWaylandSurface (surface);
	
	VulkanClient& vulkanClient = VulkanClient::instance ();
	
	// create Vulkan surface from Wayland surface
	
	VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.display = display;
	surfaceCreateInfo.surface = surface;
	VkResult result = vkCreateWaylandSurfaceKHR (vulkanClient.getVulkanInstance (), &surfaceCreateInfo, nullptr, &vulkanSurface);
	ASSERT (result == VK_SUCCESS && vulkanSurface != nullptr)
	if(vulkanSurface == nullptr)
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

bool LinuxVulkanRenderTarget::isSupported (wl_display* display, wl_surface* surface)
{
	LinuxVulkanRenderTarget target;
	return target.initialize (display, surface) && target.initializeSwapChain ();
}

//************************************************************************************************
// VulkanWindowRenderTarget
//************************************************************************************************

VulkanWindowRenderTarget::VulkanWindowRenderTarget (Window& window)
: SkiaWindowRenderTarget (window),
  linuxWindow (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanWindowRenderTarget::initialize ()
{
	linuxWindow = LinuxWindow::cast (&window);
	
	return LinuxVulkanRenderTarget::initialize (Linux::WaylandClient::instance ().getDisplay (), linuxWindow->getWaylandSurface ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::clear ()
{
	updateRegion.setEmpty ();
	invalidateRegion.setEmpty ();
	invalidateRegion.addRect (Rect (0, 0, swapChainExtent.width, swapChainExtent.height));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::onSize ()
{
	if(scaleFactor != window.getContentScaleFactor ())
		onContentScaleFactorChanged (window.getContentScaleFactor ());

	size = PixelPoint (Point (window.getWidth (), window.getHeight ()), window.getContentScaleFactor ());

	if(vulkanSurface == nullptr)
		initialize ();
	
	if(!linuxWindow->isConfigured ())
	{
		if(listener)
			delete listener;
		listener = nullptr;
		return;
	}

	if(listener == nullptr && linuxWindow->wantsFrameCallback ())
	{
		applyContentScaleFactor ();
		applySize ();
		listener = NEW Listener (this);
		onPresent ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::applySize ()
{
	surface = nullptr;
	lastSurface = nullptr;
	lastSignalSemaphore = nullptr;
	for(Vulkan3DSurface* surface : surfaces)
		surface->invalidate ();

	extent.width = size.x;
	extent.height = size.y;
	sizeChanged = true;
	
	clear ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* VulkanWindowRenderTarget::getCanvas ()
{
	SkCanvas* canvas = LinuxVulkanRenderTarget::getSkiaCanvas ();
	if(canvas == nullptr)
	{
		//TODO either the graphics device or the wayland surface is gone
	}
	return canvas;

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

void VulkanWindowRenderTarget::onRender ()
{
	if(!invalidateRegion.getRects ().isEmpty ())
	{
		AutoPtr<NativeGraphicsDevice> nativeDevice = ensureGraphicsDevice (NativeGraphicsEngine::instance ().createWindowDevice (&window));
		AutoPtr<GraphicsDevice> graphicsDevice = NEW WindowGraphicsDevice (window, nativeDevice);
		window.setGraphicsDevice (graphicsDevice);
		
		CCL_PROFILE_START (draw)
		
		// make sure to render 2D content behind transparent 3D surfaces
		for(Vulkan3DSurface* surface : surfaces)
		{
			if(surface->getContent ()->getContentHint () != kGraphicsContentTranslucent)
				continue;
			RectRef surfaceRect = surface->getSize ();
			if(invalidateRegion.rectVisible (surfaceRect))
				invalidateRegion.addRect (surfaceRect, false);
		}
		
		for(int i = 0; i < invalidateRegion.getRects ().count (); i++)
		{
			RectRef invalidateRect = invalidateRegion.getRects ().at (i);
			PixelPoint position (Point (invalidateRect.left, invalidateRect.top), window.getContentScaleFactor ());
			PixelPoint size (Point (invalidateRect.getWidth (), invalidateRect.getHeight ()), window.getContentScaleFactor ());
			int32_t x = ccl_min<uint32_t> (position.x, swapChainExtent.width);
			int32_t y = ccl_min<uint32_t> (position.y, swapChainExtent.height);
			uint32_t width = ccl_min<uint32_t> (size.x, swapChainExtent.width - x);
			uint32_t height = ccl_min<uint32_t> (size.y, swapChainExtent.height - y);
			presentRegion.add ({ VkOffset2D {x, y}, VkExtent2D {width, height}, 0 });
			
			graphicsDevice->saveState ();
			graphicsDevice->addClip (invalidateRect);
			if(isTranslucent ())
				graphicsDevice->clearRect (invalidateRect);
			
			window.setInDrawEvent (true);
			
			if(getOpacity () < 1.f)
			{
				SkPaint alpha;
				alpha.setAlphaf (getOpacity ());
				getCanvas ()->saveLayer (nullptr, &alpha);
			}
			
			window.draw (UpdateRgn (invalidateRect, &invalidateRegion));

			window.setInDrawEvent (false);
			graphicsDevice->restoreState ();
		}
		
		// blend prerendered 3D surfaces to canvas
		for(Vulkan3DSurface* surface : surfaces)
		{
			if(!surface->isValid ())
				continue;
			RectRef surfaceRect = surface->getSize ();
			if(!invalidateRegion.rectVisible (surfaceRect))
				continue;			
			sk_sp<SkImage> image = static_cast<SkiaVulkan3DSurface*> (surface)->getSkiaImage ();
			if(image)
			{
				SkRect dstRect = SkRect::MakeLTRB (surfaceRect.left, surfaceRect.top, surfaceRect.right, surfaceRect.bottom);
				getCanvas ()->drawImageRect (image, dstRect, SkSamplingOptions ());
			}
		}
		
		invalidateRegion.setEmpty ();
		
		CCL_PROFILE_STOP (draw)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::onPresent ()
{
	if(size.x != extent.width || size.y != extent.height)
		applySize ();
		
	onRender ();
	if(surface)
	{
		if(flushSurface ())
		{
			if(sizeChanged)
			{
				if(VulkanClient::instance ().getDeviceType () == VK_PHYSICAL_DEVICE_TYPE_CPU)
				{
					// Workaround for llvmpipe.
					// The first frame after resizing the surface is distorted.
					// This can be observed in third-party applications (e.g. vkcube) as well.
					// Clear and render another frame.
					if(listener)
						listener->requestFrame ();
				}
			}
		
			presentFrame ();
			nextFrame ();
			lastSurface = surface;
			lastSignalSemaphore = signalSemaphore[1];
			surface = nullptr;
		}
	}

	if(contentScaleChanged)
		applyContentScaleFactor ();

	if(IGraphicsLayer* layer = window.getGraphicsLayer ())
		layer->flush ();
	
	wl_surface_commit (getWaylandSurface ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanWindowRenderTarget::onFrameCallback ()
{
	Vector<LinuxWindow*> subSurfaces;
	linuxWindow->getSubSurfaces (subSurfaces);
	for(LinuxWindow* subSurface : subSurfaces)
	{
		VulkanWindowRenderTarget* subSurfaceRenderTarget = static_cast<VulkanWindowRenderTarget*> (subSurface->getRenderTarget ());
		if(!subSurface->isConfigured ())
		{
			subSurface->isConfigured (true);
			subSurfaceRenderTarget->onSize ();
			subSurfaceRenderTarget->applySize ();
			linuxWindow->setUserSize (linuxWindow->getSize ()); //< recalculate window bounds including the new subsurface
		}
		subSurfaceRenderTarget->onFrameCallback ();
	}

	if(invalidateRegion.getRects ().isEmpty () && updateRegion.getRects ().isEmpty ())
	{
		wl_surface_commit (getWaylandSurface ());
		return true;
	}

	onPresent ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::onScroll (RectRef rect, PointRef delta)
{
	invalidateRegion.addRect (Rect (0, 0, window.getWidth (), window.getHeight ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableRegion* VulkanWindowRenderTarget::getUpdateRegion ()
{
	return &updateRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableRegion* VulkanWindowRenderTarget::getInvalidateRegion ()
{
	return &invalidateRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::add3DSurface (Native3DSurface* surface)
{
	addVulkan3DSurface (surface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanWindowRenderTarget::remove3DSurface (Native3DSurface* surface)
{
	removeVulkan3DSurface (surface);
}

//************************************************************************************************
// VulkanLayerRenderTarget
//************************************************************************************************

VulkanLayerRenderTarget::VulkanLayerRenderTarget (wl_surface* surface, NativeGraphicsLayer& layer)
: LinuxLayerRenderTarget (surface, layer)
{
	setWaylandSurface (surface);
	setContentScaleFactor (1.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanLayerRenderTarget::initialize ()
{
	return LinuxVulkanRenderTarget::initialize (Linux::WaylandClient::instance ().getDisplay (), waylandSurface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanLayerRenderTarget::clear ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

float VulkanLayerRenderTarget::getContentScaleFactor () const
{
	return contentScaleFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* VulkanLayerRenderTarget::getCanvas ()
{
	return LinuxVulkanRenderTarget::getSkiaCanvas ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanLayerRenderTarget::setContentScaleFactor (float factor)
{
	if(contentScaleFactor != factor)
	{
		onContentScaleFactorChanged (factor);
		contentScaleFactor = factor;
		onSize ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanLayerRenderTarget::resize (RectRef newSize)
{
	if(size == newSize)
		return;
	size = newSize;
	onSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RectRef VulkanLayerRenderTarget::getSize () const
{
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanLayerRenderTarget::onSize ()
{
	pixelSize = PixelPoint (size.getSize (), contentScaleFactor);

	if(vulkanSurface == nullptr)
		initialize ();
	
	applySize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanLayerRenderTarget::applySize ()
{
	surface = nullptr;
	lastSurface = nullptr;

	extent.width = ccl_max (pixelSize.x, 1);
	extent.height = ccl_max (pixelSize.y, 1);
	sizeChanged = true;

	clear ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanLayerRenderTarget::onPresent ()
{
	if(surface)
	{
		if(flushSurface ())
		{
			presentFrame ();
			nextFrame ();
			lastSurface = nullptr;
			lastSignalSemaphore = signalSemaphore[1];
			surface = nullptr;
		}
	}

	if(contentScaleChanged)
		applyContentScaleFactor ();

	wl_surface_commit (getWaylandSurface ());
}

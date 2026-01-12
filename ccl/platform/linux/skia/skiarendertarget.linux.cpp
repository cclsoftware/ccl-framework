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
// Filename    : ccl/platform/linux/skia/skiarendertarget.linux.cpp
// Description : Skia Render Target for Linux
//
//************************************************************************************************

#include "ccl/platform/linux/skia/skiaengine.linux.h"
#include "ccl/platform/linux/skia/rasterrendertarget.h"

#if CCLGUI_VULKAN_ENABLED
	#include "ccl/platform/linux/vulkan/vulkanrendertarget.linux.h"
#endif

#if CCLGUI_OPENGLES2_ENABLED
	#include "ccl/platform/linux/opengles/openglesrendertarget.linux.h"
#endif

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// SkiaWindowRenderTarget
//************************************************************************************************

SkiaWindowRenderTarget* SkiaWindowRenderTarget::create (Window& window)
{
	LinuxSkiaEngine* engine = LinuxSkiaEngine::getInstance ();
	if(engine == nullptr)
		return nullptr;

	switch(engine->getGraphicsBackend ())
	{
	#if CCLGUI_VULKAN_ENABLED
	case LinuxSkiaEngine::kVulkan : return NEW VulkanWindowRenderTarget (window);
	#endif
	#if CCLGUI_OPENGLES2_ENABLED
	case LinuxSkiaEngine::kOpenGLES2 : return NEW OpenGLESWindowRenderTarget (window);
	#endif
	default : return NEW RasterWindowRenderTarget (window);
	}
}

//************************************************************************************************
// LinuxLayerRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (LinuxLayerRenderTarget, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxLayerRenderTarget* LinuxLayerRenderTarget::create (wl_surface* surface, NativeGraphicsLayer& layer)
{
	LinuxSkiaEngine* engine = LinuxSkiaEngine::getInstance ();
	if(engine == nullptr)
		return nullptr;

	switch(engine->getGraphicsBackend ())
	{
	#if CCLGUI_VULKAN_ENABLED
	case LinuxSkiaEngine::kVulkan : return NEW VulkanLayerRenderTarget (surface, layer);
	#endif
	#if CCLGUI_OPENGLES2_ENABLED
	case LinuxSkiaEngine::kOpenGLES2 : return NEW OpenGLESLayerRenderTarget (surface, layer);
	#endif
	default : return NEW RasterLayerRenderTarget (surface, layer);
	}
}

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
// Filename    : ccl/platform/linux/skia/skiaengine.linux.cpp
// Description : Linux Skia Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/skia/skiaengine.linux.h"
#include "ccl/platform/linux/skia/skialayer.linux.h"

#include "ccl/platform/shared/skia/skiabitmap.h"

#if CCLGUI_VULKAN_ENABLED
	#include "ccl/platform/shared/vulkan/vulkanskia3dsupport.h"
#endif
#if CCLGUI_OPENGLES2_ENABLED
	#include "ccl/platform/shared/opengles/opengles3dsupport.h"
	#include "ccl/platform/shared/opengles/openglesclient.h"
#endif

#include "ccl/base/storage/configuration.h"

#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxSkiaEngine
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (LinuxSkiaEngine, SkiaEngine)

#if CCLGUI_VULKAN_ENABLED
const Configuration::BoolValue useVulkan ("CCL.Graphics.Vulkan", "Enabled", true);
#endif
#if CCLGUI_OPENGLES2_ENABLED
const Configuration::BoolValue useOpenGLES2 ("CCL.Graphics.OpenGLES2", "Enabled", true);
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxSkiaEngine* LinuxSkiaEngine::getInstance ()
{
	return ccl_cast<LinuxSkiaEngine> (&NativeGraphicsEngine::instance ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxSkiaEngine::GraphicsBackendType LinuxSkiaEngine::getGraphicsBackend () const
{
	#if CCLGUI_VULKAN_ENABLED
	if(useVulkan && VulkanClient::instance ().isSupported ())
		return kVulkan;
	#endif
	#if CCLGUI_OPENGLES2_ENABLED
	if(useOpenGLES2 && OpenGLESClient::instance ().isSupported ())
		return kOpenGLES2;
	#endif
	return kSoftware;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsFactory* LinuxSkiaEngine::create3DGraphicsFactory ()
{
	switch(getGraphicsBackend ())
	{
	#if CCLGUI_VULKAN_ENABLED
	case kVulkan : return NEW Vulkan3DGraphicsFactory;
	#endif
	#if CCLGUI_OPENGLES2_ENABLED
	case kOpenGLES2 : return NEW OpenGLES3DGraphicsFactory;
	#endif
	default : return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxSkiaEngine::hasGraphicsLayers ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* LinuxSkiaEngine::createGraphicsLayer (UIDRef classID)
{
	return SkiaLayerFactory::createLayer (classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* LinuxSkiaEngine::createScreenshotFromWindow (Window* window)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* LinuxSkiaEngine::createPrintJob ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INative3DSupport* LinuxSkiaEngine::get3DSupport ()
{
	switch(getGraphicsBackend ())
	{
	#if CCLGUI_VULKAN_ENABLED
	case kVulkan : return &SkiaVulkan3DSupport::instance ();
	#endif
	#if CCLGUI_OPENGLES2_ENABLED
	case kOpenGLES2 : return &OpenGLES3DSupport::instance ();
	#endif
	default : return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GrRecordingContext* LinuxSkiaEngine::getGPUContext ()
{
	switch(getGraphicsBackend ())
	{
	#if CCLGUI_VULKAN_ENABLED
	case kVulkan : return VulkanClient::instance ().getGPUContext ();
	#endif
	#if CCLGUI_OPENGLES2_ENABLED
	case kOpenGLES2 : return OpenGLESClient::instance ().getGPUContext ();
	#endif
	default : return nullptr;
	}
}

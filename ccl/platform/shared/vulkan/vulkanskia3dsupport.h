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
// Filename    : ccl/platform/shared/vulkan/vulkanskia3dsupport.h
// Description : Vulkan Skia 3D Support
//
//************************************************************************************************

#ifndef _vulkanskia3dsupport_h
#define _vulkanskia3dsupport_h

#include "ccl/platform/shared/vulkan/vulkan3dsupport.h"

#include "ccl/platform/shared/skia/skiaglue.h"

namespace CCL {

//************************************************************************************************
// SkiaVulkan3DSurface
//************************************************************************************************

class SkiaVulkan3DSurface: public Vulkan3DSurface
{
public:
	DECLARE_CLASS (SkiaVulkan3DSurface, Vulkan3DSurface)

	sk_sp<SkImage> getSkiaImage () const;

	// Vulkan3DSurface
	bool create (VulkanGPUContext* gpuContext, VkFormat format, float scaleFactor, int imageCount) override;
	void destroy () override;
	bool isValid () const override;

protected:
	Vector<sk_sp<SkImage>> skiaImages;
};

//************************************************************************************************
// SkiaVulkanClient
//************************************************************************************************

class SkiaVulkanClient: public VulkanClient
{
public:
	bool initializeLogicalDevice () override;
	void terminate () override;

	VulkanGPUContext* getGPUContext () override;
	
protected:
	sk_sp<GrDirectContext> gpuContext;
};

//************************************************************************************************
// SkiaVulkan3DSupport
//************************************************************************************************

class SkiaVulkan3DSupport: public INative3DSupport,
						   public StaticSingleton<SkiaVulkan3DSupport>
{
public:
	// INative3DSupport
	Native3DGraphicsFactory& get3DFactory () override;
	Native3DSurface* create3DSurface () override;
};

} // namespace CCL

#endif // _vulkanskia3dsupport_h

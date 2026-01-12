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
// Filename    : ccl/platform/shared/vulkan/vulkanskiarendertarget.h
// Description : Vulkan Skia Render Target
//
//************************************************************************************************

#ifndef _ccl_vulkanskiarendertarget_h
#define _ccl_vulkanskiarendertarget_h

#include "ccl/platform/shared/skia/skiaglue.h"
#include "ccl/platform/shared/vulkan/vulkanrendertarget.h"

class SkCanvas;

namespace CCL {

//************************************************************************************************
// SkiaVulkanRenderTarget
//************************************************************************************************

class SkiaVulkanRenderTarget: public VulkanRenderTarget
{	
protected:
	sk_sp<SkSurface> lastSurface;
	
	virtual sk_sp<SkSurface> getSurface () { return nullptr; }
	virtual void setSurface (sk_sp<SkSurface> surface) {}
	virtual SkCanvas* getSkiaCanvas ();

	// VulkanRenderTarget
	void reinitialize (InitializeLevel level = InitializeLevel::kDevice) override;

	bool flushSurface () override;
};

} // namespace CCL

#endif // ccl_vulkanskiarendertarget_h

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
// Filename    : ccl/platform/android/graphics/android3dsupport.h
// Description : Android 3D Support
//
//************************************************************************************************

#ifndef _ccl_android3dsupport_h
#define _ccl_android3dsupport_h

#include "ccl/platform/shared/vulkan/vulkan3dsupport.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// Android3DSurface
//************************************************************************************************

class Android3DSurface: public Vulkan3DSurface
{
public:
	DECLARE_CLASS (Android3DSurface, Vulkan3DSurface)

	VkImage getVulkanImage () const;
};

//************************************************************************************************
// Android3DSupport
//************************************************************************************************

class Android3DSupport: public INative3DSupport,
						public StaticSingleton<Android3DSupport>
{
public:
	// INative3DSupport
	Native3DGraphicsFactory& get3DFactory () override;
	Native3DSurface* create3DSurface () override;
};

} // namespace Android
} // namespace CCL

#endif // _ccl_android3dsupport_h

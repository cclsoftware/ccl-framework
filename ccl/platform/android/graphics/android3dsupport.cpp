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
// Filename    : ccl/platform/android/graphics/android3dsupport.cpp
// Description : Android 3D Support
//
//************************************************************************************************

#include "ccl/platform/android/graphics/android3dsupport.h"

using namespace CCL;
using namespace Android;

//************************************************************************************************
// Vulkan3DGraphicsFactory
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (Native3DGraphicsFactory, Vulkan3DGraphicsFactory)

//************************************************************************************************
// Android3DSurface
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Android3DSurface, Vulkan3DSurface)

//////////////////////////////////////////////////////////////////////////////////////////////////

VkImage Android3DSurface::getVulkanImage () const
{
	return getResolveImage ();
}

//************************************************************************************************
// Android3DSupport
//************************************************************************************************

Native3DGraphicsFactory& Android3DSupport::get3DFactory ()
{
	return Vulkan3DSupport::instance ().get3DFactory ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DSurface* Android3DSupport::create3DSurface ()
{
	return NEW Android3DSurface;
}

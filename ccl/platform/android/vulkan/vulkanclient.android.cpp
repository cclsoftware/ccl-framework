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
// Filename    : ccl/platform/android/vulkan/vulkanclient.android.cpp
// Description : Vulkan Client Context for Android
//
//************************************************************************************************

#include "ccl/platform/android/vulkan/vulkanrendertarget.android.h"

#include <vulkan/vulkan_android.h>

namespace CCL {
namespace Android {
    
//************************************************************************************************
// AndroidVulkanClient
//************************************************************************************************

class AndroidVulkanClient: public VulkanClient
{
protected:
    static const Vector<CStringPtr> kRequiredPlatformExtensions;

    // VulkanClient
    bool initializePlatform () override;
    const Vector<CStringPtr>& getRequiredPlatformExtensions () const override;
};
    
} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// AndroidVulkanClient
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (VulkanClient, AndroidVulkanClient)

const Vector<CStringPtr> AndroidVulkanClient::kRequiredPlatformExtensions =
{
	VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
};

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<CStringPtr>& AndroidVulkanClient::getRequiredPlatformExtensions () const
{ 
	return kRequiredPlatformExtensions;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidVulkanClient::initializePlatform ()
{
	return AndroidVulkanRenderTarget::isSupported ();
}

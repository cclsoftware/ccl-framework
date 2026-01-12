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
// Filename    : ccl/platform/linux/vulkan/vulkanclient.linux.cpp
// Description : Vulkan Client Context using Wayland
//
//************************************************************************************************

#include "ccl/platform/shared/vulkan/vulkanskia3dsupport.h"

#include "ccl/platform/linux/vulkan/vulkanrendertarget.linux.h"

#include <vulkan/vulkan_wayland.h>

namespace CCL {
namespace Linux {
    
//************************************************************************************************
// LinuxVulkanClient
//************************************************************************************************

class LinuxVulkanClient: public SkiaVulkanClient
{
protected:
    static const Vector<CStringPtr> kRequiredPlatformExtensions;

    // VulkanClient
    bool initializePlatform () override;
    const Vector<CStringPtr>& getRequiredPlatformExtensions () const override;
};
    
} // namespace Linux
} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxVulkanClient
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (VulkanClient, LinuxVulkanClient)

const Vector<CStringPtr> LinuxVulkanClient::kRequiredPlatformExtensions = 
{ 
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME 
};

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<CStringPtr>& LinuxVulkanClient::getRequiredPlatformExtensions () const
{ 
	return kRequiredPlatformExtensions;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxVulkanClient::initializePlatform ()
{
	bool result = false;
	
	// try to initialize with a wayland surface
	wl_compositor* waylandCompositor = WaylandClient::instance ().getCompositor ();
	if(waylandCompositor)
	{
		wl_display* waylandDisplay = WaylandClient::instance ().getDisplay ();
		wl_surface* waylandSurface = wl_compositor_create_surface (waylandCompositor);
			
		if(waylandSurface)
		{
			do
			{
				if(LinuxVulkanRenderTarget::isSupported (waylandDisplay, waylandSurface))
					result = true;
				else
				{
					deviceCandidates.remove (physicalDevice);
					terminate ();
				}
			} while (result == false && !deviceCandidates.isEmpty ());
		}
		wl_surface_destroy (waylandSurface);
    }
    
    if(result == false)
        CCL_WARN ("%s\n", "Vulkan/Wayland is not supported!")
    
    return result;
}
 

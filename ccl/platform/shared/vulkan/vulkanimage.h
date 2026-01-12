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
// Filename    : ccl/platform/shared/vulkan/vulkanimage.h
// Description : Vulkan Image
//
//************************************************************************************************

#ifndef _vulkanimage_h
#define _vulkanimage_h

#include "ccl/platform/shared/vulkan/vulkanclient.h"

namespace CCL {

//************************************************************************************************
// VulkanImage
//************************************************************************************************

class VulkanImage
{
public:
	static VkFormat kNativeImageFormat;

	VulkanImage ();
	~VulkanImage ();

	PROPERTY_VARIABLE (VkExtent2D, size, Size)
	PROPERTY_VARIABLE (VkSampleCountFlagBits, sampleCount, SampleCount)
	PROPERTY_VARIABLE (VkFormat, format, Format)
	PROPERTY_VARIABLE (VkImageTiling, tiling, Tiling)
	PROPERTY_VARIABLE (VkFlags, usage, Usage)
	PROPERTY_VARIABLE (VkFlags, aspect, Aspect)
	PROPERTY_VARIABLE (VkImageLayout, layout, Layout)
	PROPERTY_VARIABLE (int, mipLevels, MipLevels)

	VkImage getImage () const { return image; }
	VkImageView getImageView () const { return imageView; }

	bool create ();
	bool createFromExisting (VkImage image);
	void destroy ();
	
	bool generateMipmaps (VkCommandBuffer commandBuffer);
	void transition (VkCommandBuffer commandBuffer, VkImageLayout srcLayout, VkImageLayout dstLayout);

protected:
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;

	bool createImageView ();
};

} // namespace CCL

#endif // _vulkanimage_h


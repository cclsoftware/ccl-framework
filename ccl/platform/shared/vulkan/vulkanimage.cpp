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
// Filename    : ccl/platform/shared/vulkan/vulkanimage.cpp
// Description : Vulkan Image
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/vulkan/vulkanimage.h"

#include "ccl/public/gui/graphics/ibitmap.h"

#include <vulkan/vk_enum_string_helper.h>

using namespace CCL;

//************************************************************************************************
// VulkanImage
//************************************************************************************************

#if (CORE_BITMAP_PLATFORM_FORMAT == CORE_BITMAP_FORMAT_RGBA)
VkFormat VulkanImage::kNativeImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
#else
VkFormat VulkanImage::kNativeImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

VulkanImage::VulkanImage ()
: size {0, 0},
  sampleCount (VK_SAMPLE_COUNT_1_BIT),
  format (kNativeImageFormat),
  tiling (VK_IMAGE_TILING_OPTIMAL),
  usage (0),
  aspect (VK_IMAGE_ASPECT_COLOR_BIT),
  image (VK_NULL_HANDLE),
  imageMemory (VK_NULL_HANDLE),
  imageView (VK_NULL_HANDLE),
  layout (VK_IMAGE_LAYOUT_UNDEFINED),
  mipLevels (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

VulkanImage::~VulkanImage ()
{
	destroy ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanImage::create ()
{
	ASSERT (image == VK_NULL_HANDLE)
	ASSERT (imageMemory == VK_NULL_HANDLE)
	ASSERT (imageView == VK_NULL_HANDLE)

	ASSERT (mipLevels > 0)

	VkImageCreateInfo imageInfo {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = size.width;
	imageInfo.extent.height = size.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = layout;
	imageInfo.usage = usage;
	if(mipLevels > 1)
		imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageInfo.samples = sampleCount;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VulkanClient& vulkanClient = VulkanClient::instance ();
	VkDevice device = vulkanClient.getLogicalDevice ();
	VkPhysicalDevice physicalDevice = vulkanClient.getPhysicalDevice ();

	VkResult result = vkCreateImage (device, &imageInfo, nullptr, &image);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s %s\n", "Failed to create a GPU image.", string_VkResult (result))
		return false;
	}

	VkMemoryRequirements memoryRequirements {};
	vkGetImageMemoryRequirements (device, image, &memoryRequirements);

	VkPhysicalDeviceMemoryProperties memoryProperties {};
	vkGetPhysicalDeviceMemoryProperties (physicalDevice, &memoryProperties);

	VkMemoryAllocateInfo imageAllocInfo {};
	imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageAllocInfo.allocationSize = memoryRequirements.size;

	for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if(get_flag<uint32_t> (memoryRequirements.memoryTypeBits, (1 << i)))
		{
			imageAllocInfo.memoryTypeIndex = i;
			break;
		}
	}

	result = vkAllocateMemory (device, &imageAllocInfo, nullptr, &imageMemory);
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s %s\n", "Failed to allocate memory for a GPU image.", string_VkResult (result))
		return false;
	}

	result = vkBindImageMemory (device, image, imageMemory, 0);

	return createImageView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanImage::createFromExisting (VkImage existingImage)
{
	ASSERT (image == VK_NULL_HANDLE)
	ASSERT (imageMemory == VK_NULL_HANDLE)
	ASSERT (imageView == VK_NULL_HANDLE)

	image = existingImage;

	return createImageView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanImage::createImageView ()
{
	VulkanClient& vulkanClient = VulkanClient::instance ();
	VkDevice device = vulkanClient.getLogicalDevice ();

	VkImageViewCreateInfo viewInfo {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspect;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkResult result = vkCreateImageView (device, &viewInfo, nullptr, &imageView);
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s %s\n", "Failed to create a GPU image view.", string_VkResult (result))
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanImage::generateMipmaps (VkCommandBuffer commandBuffer)
{
	VulkanClient& vulkanClient = VulkanClient::instance ();
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties (vulkanClient.getPhysicalDevice (), format, &formatProperties);
	if(!get_flag<VkFlags> (formatProperties.optimalTilingFeatures, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		CCL_WARN ("%s\n", "Failed to generate mipmaps. Image format does not support linear filtering!")
		return false;
	}

	VkImageMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	VkOffset3D mipSize = { int32_t (size.width), int32_t (size.height), 1 };
	for(int i = 1; i < mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier (commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipSize.x, mipSize.y, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipSize.x > 1 ? mipSize.x / 2 : 1, mipSize.y > 1 ? mipSize.y / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage (commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier (commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier);

		if(mipSize.x > 1)
			mipSize.x /= 2;
		if(mipSize.y > 1)
			mipSize.y /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier (commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr, 0, nullptr, 1, &barrier);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanImage::destroy ()
{
	if(image == VK_NULL_HANDLE)
		return;
	
	VulkanClient& vulkanClient = VulkanClient::instance ();
	VkDevice device = vulkanClient.getLogicalDevice ();

	if(imageView)
		vkDestroyImageView (device, imageView, nullptr);
	imageView = VK_NULL_HANDLE;

	if(image)
		vkDestroyImage (device, image, nullptr);
	image = VK_NULL_HANDLE;

	if(imageMemory)
		vkFreeMemory (device, imageMemory, nullptr);
	imageMemory = VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanImage::transition (VkCommandBuffer commandBuffer, VkImageLayout srcLayout, VkImageLayout dstLayout)
{
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.oldLayout = srcLayout;
	imageMemoryBarrier.newLayout = dstLayout;
	imageMemoryBarrier.image = getImage ();
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
	
	VkPipelineStageFlagBits srcStage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
	VkPipelineStageFlagBits dstStage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
	
	if(srcLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		// e.g. preparing an image for shader read after rendering content to it
		
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if(srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		// e.g. preparing a texture for copying the staging buffer to it
		
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if(srcLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		// e.g. preparing a texture for shader use after copying the staging buffer to it
		
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;		
	}
	
	vkCmdPipelineBarrier (commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

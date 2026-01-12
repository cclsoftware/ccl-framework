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
// Filename    : ccl/platform/shared/vulkan/vulkan3dsupport.cpp
// Description : Vulkan 3D Support
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/vulkan/vulkan3dsupport.h"
#include "ccl/platform/shared/opengles/glslshaderreflection.h"

#include "ccl/base/storage/file.h"

#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/public/gui/graphics/3d/stockshader3d.h"
#include "ccl/public/gui/graphics/3d/vertex3d.h"
#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/public/math/mathprimitives.h"

#include <vulkan/vk_enum_string_helper.h>

namespace CCL {

//************************************************************************************************
// VulkanFormatMap
//************************************************************************************************

struct VulkanFormatMap
{
	DataFormat3D format;
	VkFormat vulkanFormat;
	int size;
};
static constexpr VulkanFormatMap kVulkanFormatMap[] =
{
	{ kR8_Int,             VK_FORMAT_R8_SINT              , 1 },
	{ kR8_UInt,            VK_FORMAT_R8_UINT              , 1 },
	{ kR16_Int,            VK_FORMAT_R16_SINT             , 2 },
	{ kR16_UInt,           VK_FORMAT_R16_UINT             , 2 },
	{ kR32_Int,            VK_FORMAT_R32_SINT             , 4 },
	{ kR32_UInt,           VK_FORMAT_R32_UINT             , 4 },
	{ kR32_Float,          VK_FORMAT_R32_SFLOAT           , 4 },
	{ kR8G8_Int,           VK_FORMAT_R8G8_SINT            , 2 },
	{ kR8G8_UInt,          VK_FORMAT_R8G8_UINT            , 2 },
	{ kR16G16_Int,         VK_FORMAT_R16G16_SINT          , 4 },
	{ kR16G16_UInt,        VK_FORMAT_R16G16_UINT          , 4 },
	{ kR32G32_Int,         VK_FORMAT_R32G32_SINT          , 8 },
	{ kR32G32_UInt,        VK_FORMAT_R32G32_UINT          , 8 },
	{ kR32G32_Float,       VK_FORMAT_R32G32_SFLOAT        , 8 },
	{ kR32G32B32_Int,      VK_FORMAT_R32G32B32_SINT       , 12},
	{ kR32G32B32_UInt,     VK_FORMAT_R32G32B32_UINT       , 12},
	{ kR32G32B32_Float,    VK_FORMAT_R32G32B32_SFLOAT     , 12},
	{ kR32G32B32A32_Int,   VK_FORMAT_R32G32B32A32_SINT    , 16},
	{ kR32G32B32A32_UInt,  VK_FORMAT_R32G32B32A32_UINT    , 16},
	{ kR8G8B8A8_UNORM,     VK_FORMAT_R8G8B8A8_UNORM        , 4},
	{ kB8G8R8A8_UNORM,     VK_FORMAT_B8G8R8A8_UNORM        , 4}
};

static constexpr VkFormat getVulkanFormat (DataFormat3D format)
{
	for(const auto& entry : kVulkanFormatMap)
	{
		if(entry.format == format)
			return entry.vulkanFormat;
	}

	return VK_FORMAT_UNDEFINED;
}

static constexpr int getVulkanFormatSize (DataFormat3D format)
{
	for(const auto& entry : kVulkanFormatMap)
	{
		if(entry.format == format)
			return entry.size;
	}

	return 0;
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Vulkan3DSurface
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Vulkan3DSurface, Native3DSurface)

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DSurface::Vulkan3DSurface ()
: currentCommandBuffer (-1),
  renderpass (VK_NULL_HANDLE),
  sampleCount (VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM),
  scaleFactor (1.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DSurface::~Vulkan3DSurface ()
{
	destroy ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DSurface::setContent (IGraphicsContent3D* _content)
{
	SuperClass::setContent (_content);
	int _sampleCount = content ? content->getMultisampling () : 1;
	if(_sampleCount != sampleCount)
	{
		destroy (); // Vulkan objects need to be recreated. The render target will call create in the next render call.
		applyMultisampling (_sampleCount);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DSurface::setSize (const Rect& _size)
{
	SuperClass::setSize (_size);
	destroy (); // Vulkan objects need to be recreated. The render target will call create in the next render call.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DSurface::applyMultisampling (int samples)
{
	samples = ccl_upperPowerOf2 (samples / scaleFactor);

	VkPhysicalDevice physicalDevice = VulkanClient::instance ().getPhysicalDevice ();
	
	// check multisampling support

	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties (physicalDevice, &physicalDeviceProperties);
	VkSampleCountFlags maxSamples = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	sampleCount = static_cast<VkSampleCountFlagBits> (samples);
	while((sampleCount & maxSamples) == 0 && sampleCount != 0)
	{
		sampleCount = static_cast<VkSampleCountFlagBits> (sampleCount >> 1);
	}
	if(sampleCount == 0)
		sampleCount = VK_SAMPLE_COUNT_1_BIT;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Vulkan3DSurface::create (VulkanGPUContext* gpuContext, VkFormat format, float _scaleFactor, int count)
{
	ASSERT (commandBuffers.isEmpty ())
	if(!commandBuffers.isEmpty ())
		return false;
	
	ASSERT (count > 0)
	if(count <= 0)
		return false;
	
	scaleFactor = _scaleFactor;
	
	if(content)
		applyMultisampling (content->getMultisampling ());

	VkDevice device = VulkanClient::instance ().getLogicalDevice ();
	VkPhysicalDevice physicalDevice = VulkanClient::instance ().getPhysicalDevice ();
	
	VkCommandBufferAllocateInfo allocInfo {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = VulkanClient::instance ().getCommandPool ();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;

	commandBuffers.setCount (count);
	commandBuffers.zeroFill ();
	
	VkResult result = vkAllocateCommandBuffers (device, &allocInfo, commandBuffers);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s %s\n", "Failed to allocate command buffers for 3D surface.", string_VkResult (result))
		destroy ();
		return false;
	}

	// check physical device formats for the depth buffer

	VkFormat depthBufferFormat = VK_FORMAT_UNDEFINED;

	for(const VkFormat& candidate : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT})
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties (physicalDevice, candidate, &properties);

		if(get_flag<VkFlags> (properties.optimalTilingFeatures, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			depthBufferFormat = candidate;
			break;
		}
	}

	if(depthBufferFormat == VK_FORMAT_UNDEFINED)
	{
		CCL_WARN ("%s\n", "No matching depth buffer format available.")
		destroy ();
		return false;
	}

	// create a color image for multisampling

	viewPortRect = PixelRect (size, scaleFactor);

	VkExtent2D extent {};
	extent.width = viewPortRect.getWidth ();
	extent.height = viewPortRect.getHeight ();

	bool multisamplingEnabled = (sampleCount != VK_SAMPLE_COUNT_1_BIT);
	if(multisamplingEnabled)
	{
		colorImage.setSize (extent);
		colorImage.setFormat (format);
		colorImage.setUsage (VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		colorImage.setAspect (VK_IMAGE_ASPECT_COLOR_BIT);
		colorImage.setSampleCount (sampleCount);
		if(!colorImage.create ())
		{
			CCL_WARN ("%s\n", "Failed to create a multisampling buffer.")
			destroy ();
			return false;
		}
	}

	// create a depth buffer image

	depthImage.setSize (extent);
	depthImage.setFormat (depthBufferFormat);
	depthImage.setUsage (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depthImage.setAspect (VK_IMAGE_ASPECT_DEPTH_BIT);
	depthImage.setSampleCount (sampleCount);
	if(!depthImage.create ())
	{
		CCL_WARN ("%s\n", "Failed to create a depth buffer.")
		destroy ();
		return false;
	}

	// create a render pass
	
	VkAttachmentLoadOp colorLoadOp = hasClearColor () ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;

	VkAttachmentDescription colorAttachment {};
	colorAttachment.format = format;
	colorAttachment.samples = sampleCount;
	colorAttachment.loadOp = colorLoadOp;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment {};
	depthAttachment.format = depthBufferFormat;
	depthAttachment.samples = sampleCount;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve {};
	colorAttachmentResolve.format = format;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = multisamplingEnabled ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : colorLoadOp;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription renderSubpass {};
	renderSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	renderSubpass.colorAttachmentCount = 1;
	renderSubpass.pColorAttachments = &colorAttachmentRef;
	renderSubpass.pDepthStencilAttachment = &depthAttachmentRef;
	renderSubpass.pResolveAttachments = multisamplingEnabled ? &colorAttachmentResolveRef : nullptr;

	VkSubpassDescription subpasses[] = { renderSubpass };
	
	VkAttachmentDescription attachments[] = { multisamplingEnabled ? colorAttachment : colorAttachmentResolve, depthAttachment, colorAttachmentResolve };
	
	VkRenderPassCreateInfo renderPassInfo {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = multisamplingEnabled ? 3 : 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = ARRAY_COUNT (subpasses);
	renderPassInfo.pSubpasses = subpasses;

	VkSubpassDependency renderDependency {};
	renderDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	renderDependency.dstSubpass = 0;
	renderDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	renderDependency.srcAccessMask = 0;
	renderDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	renderDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	
	VkSubpassDependency dependencies[] = { renderDependency };
	renderPassInfo.dependencyCount = ARRAY_COUNT (dependencies);
	renderPassInfo.pDependencies = dependencies;

	result = vkCreateRenderPass (device, &renderPassInfo, nullptr, &renderpass);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s %s\n", "Failed to create a render pass.", string_VkResult (result))
		destroy ();
		return false;
	}

	// create image views

	resolveImages.setCount (count);
	framebuffers.setCount (count);
	framebuffers.zeroFill ();
	for(int i = 0; i < count; i++)
	{
		resolveImages[i].setSize (extent);
		resolveImages[i].setFormat (format);
		resolveImages[i].setUsage (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT); // transfer and sampled bits are required by Skia
		resolveImages[i].setAspect (VK_IMAGE_ASPECT_COLOR_BIT);
		resolveImages[i].setSampleCount (VK_SAMPLE_COUNT_1_BIT);
		if(!resolveImages[i].create ())
		{
			CCL_WARN ("%s\n", "Failed to create a resolve buffer.")
			destroy ();
			return false;
		}

		VkImageView imageView = resolveImages[i].getImageView ();

		VkImageView attachments[] = { multisamplingEnabled ? colorImage.getImageView () : imageView, depthImage.getImageView (), imageView };

		VkFramebufferCreateInfo framebufferInfo {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderpass;
		framebufferInfo.attachmentCount = multisamplingEnabled ? 3 : 2;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		result = vkCreateFramebuffer (device, &framebufferInfo, nullptr, &framebuffers[i]);
		ASSERT (result == VK_SUCCESS)
		if(result != VK_SUCCESS)
		{
			CCL_WARN ("%s %s\n", "Failed to create frame buffer from existing image view.", string_VkResult (result))
			destroy ();
			return false;
		}
	}

	// Create semaphores
	
	signalSemaphores.setCount (count);

	VkSemaphoreCreateInfo semaphoreCreateInfo {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for(int i = 0; i < count; i++)
	{
		result = vkCreateSemaphore (device, &semaphoreCreateInfo, nullptr, &signalSemaphores[i]);
		ASSERT (result == VK_SUCCESS)
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DSurface::destroy ()
{
	VkDevice device = VulkanClient::instance ().getLogicalDevice ();

	vkDeviceWaitIdle (device);
		
	for(int i = 0; i < signalSemaphores.count (); i++)
		vkDestroySemaphore (device, signalSemaphores[i], nullptr);
	signalSemaphores.setCount (0);

	if(!commandBuffers.isEmpty ())
	{
		vkFreeCommandBuffers (device, VulkanClient::instance ().getCommandPool (), commandBuffers.count (), commandBuffers);
		
		commandBuffers.removeAll ();
		currentCommandBuffer = -1;
	}

	for(int i = 0; i < framebuffers.count (); i++)
        vkDestroyFramebuffer (device, framebuffers[i], nullptr);
	framebuffers.setCount (0);

	colorImage.destroy ();
	depthImage.destroy ();
	for(auto& image : resolveImages)
		image.destroy ();

	if(renderpass != VK_NULL_HANDLE)
		vkDestroyRenderPass (device, renderpass, nullptr);
	renderpass = VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DSurface::invalidate ()
{
	currentCommandBuffer = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Vulkan3DSurface::isValid () const
{
	return !commandBuffers.isEmpty () && renderpass != VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VkSemaphore Vulkan3DSurface::render (Vulkan3DGraphicsContext& context)
{
	ASSERT (renderpass != VK_NULL_HANDLE)
	
	if(!isDirty () && currentCommandBuffer >= 0)
	{
		// The surface contents did not change. Just return and let the frontend draw the prerendered image.
		
		return VK_NULL_HANDLE;
	}
	
	// This surface is dirty. We need to record draw commands.

	VkCommandBuffer commandBuffer = nextCommandBuffer ();
	ASSERT (commandBuffer != nullptr)
	if(commandBuffer == nullptr)
		return VK_NULL_HANDLE;

	VkCommandBufferBeginInfo beginInfo {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	vkResetCommandBuffer (commandBuffer, 0);
	VkResult result = vkBeginCommandBuffer (commandBuffer, &beginInfo);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s: %s\n", "Failed to record primary command buffer for 3D surface", string_VkResult (result))
		return VK_NULL_HANDLE;
	}
	
	context.setRenderpass (renderpass);
	context.setSampleCount (colorImage.getSampleCount ());
	context.setCommandBuffer (commandBuffer);

	VkExtent2D extent {};
	extent.width = viewPortRect.getWidth ();
	extent.height = viewPortRect.getHeight ();

	ColorF clearColor (getClearColor ());
	#if 0 && DEBUG
	clearColor = ColorF (0.f, 0.f, 0.2f, 1.f);
	#endif
	VkClearValue colorClearValue {};
	colorClearValue.color.float32[0] = clearColor.red;
	colorClearValue.color.float32[1] = clearColor.green;
	colorClearValue.color.float32[2] = clearColor.blue;
	colorClearValue.color.float32[3] = clearColor.alpha;
	VkClearValue depthClearValue {};
	depthClearValue.depthStencil = {1.0f, 0};
	VkClearValue clearValues[3] = {colorClearValue, depthClearValue, colorClearValue};

	VkRenderPassBeginInfo renderPassInfo {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderpass;
	renderPassInfo.framebuffer = getFrameBuffer ();
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = extent;
	renderPassInfo.clearValueCount = ARRAY_COUNT (clearValues);
	renderPassInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass (commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	context.setViewport (Rect (0, 0, extent.width, extent.height));
	if(IGraphicsContent3D* content = getContent ())
	{
		content->renderContent (context);
		setDirty (false);
	}

	vkCmdEndRenderPass (commandBuffer);
	
	resolveImages[currentCommandBuffer].transition (commandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	result = vkEndCommandBuffer (commandBuffer);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
		return VK_NULL_HANDLE;

	VkSubmitInfo submitInfo {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore semaphores[] = { getSemaphore () };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = semaphores;

	result = vkQueueSubmit (VulkanClient::instance ().getGraphicsQueue (), 1, &submitInfo, VK_NULL_HANDLE);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
		return VK_NULL_HANDLE;

	return getSemaphore ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VkImage Vulkan3DSurface::getResolveImage () const
{
	return currentCommandBuffer >= 0 ? resolveImages[currentCommandBuffer].getImage () : VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VkFramebuffer Vulkan3DSurface::getFrameBuffer () const
{
	return currentCommandBuffer >= 0 ? framebuffers[currentCommandBuffer] : VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer Vulkan3DSurface::getCommandBuffer () const
{
	return currentCommandBuffer >= 0 ? commandBuffers[currentCommandBuffer] : VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VkSemaphore Vulkan3DSurface::getSemaphore () const
{
	return currentCommandBuffer >= 0 ? signalSemaphores[currentCommandBuffer] : VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer Vulkan3DSurface::nextCommandBuffer ()
{
	currentCommandBuffer = (currentCommandBuffer + 1) % commandBuffers.count ();
	return getCommandBuffer ();
}

//************************************************************************************************
// Vulkan3DSupport
//************************************************************************************************

void Vulkan3DSupport::shutdown3D ()
{
	Vulkan3DResourceManager::instance ().shutdown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsFactory& Vulkan3DSupport::get3DFactory ()
{
	return factory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DSurface* Vulkan3DSupport::create3DSurface ()
{
	return NEW Vulkan3DSurface;
}

//************************************************************************************************
// Vulkan3DVertexFormat
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Vulkan3DVertexFormat, Native3DVertexFormat)

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DVertexFormat::Vulkan3DVertexFormat ()
: vertexInputInfo {},
  bindingDescription {}
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Vulkan3DVertexFormat::create (const VertexElementDescription description[], uint32 count, const IGraphicsShader3D* shader)
{
	attributeDescription.setCount (count);
	
	bindingDescription.binding = 0;
	bindingDescription.stride = 0;
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		
	for(uint32 i = 0; i < attributeDescription.count (); i++)
	{
		attributeDescription[i].binding = bindingDescription.binding;
		attributeDescription[i].format = getVulkanFormat (description[i].format);
		attributeDescription[i].location = i;
		attributeDescription[i].offset = bindingDescription.stride;
		
		bindingDescription.stride += getVulkanFormatSize (description[i].format);
		
		ASSERT (attributeDescription[i].format != VK_FORMAT_UNDEFINED)
	}
	
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // optional
	vertexInputInfo.vertexAttributeDescriptionCount = count;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescription; // Optional
	
	return true;
}

//************************************************************************************************
// Vulkan3DBuffer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Vulkan3DBuffer, Native3DGraphicsBuffer)

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DBuffer::Vulkan3DBuffer ()
: bufferInfo {},
  buffer (VK_NULL_HANDLE),
  memory (VK_NULL_HANDLE),
  memoryAlignment (1),
  mapCount (0),
  mappedData (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DBuffer::~Vulkan3DBuffer ()
{
	ASSERT (mapCount == 0)
	destroy ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DBuffer::destroy ()
{
	if(buffer == VK_NULL_HANDLE && memory == VK_NULL_HANDLE)
		return;

	VkDevice device = VulkanClient::instance ().getLogicalDevice ();

	if(mapCount == 0 && mappedData != nullptr)
	{
		vkUnmapMemory (device, memory);
		mappedData = nullptr;
	}

	vkDeviceWaitIdle (device);
			
	if(buffer)
		vkDestroyBuffer (device, buffer, nullptr);
	buffer = VK_NULL_HANDLE;

	if(memory)
		vkFreeMemory (device, memory, nullptr);
	memory = VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Vulkan3DBuffer::create (Type _type, BufferUsage3D usage, uint32 sizeInBytes, uint32 strideInBytes, const void* initialData)
{
	destroy ();

	type = _type;
	
	switch(type)
	{
	case kConstantBuffer :
		{	
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties (VulkanClient::instance ().getPhysicalDevice (), &deviceProperties);
			memoryAlignment = uint32 (deviceProperties.limits.minUniformBufferOffsetAlignment);
		}
		break;
	}
	
	uint32 offset = 0;
	if(!ensureSegmentAlignment (offset, sizeInBytes, strideInBytes))
		return false;
	
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeInBytes;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // UNSURE
	bufferInfo.usage = 0;
	bufferInfo.flags = 0; // UNSURE
	
	switch(_type)
	{
	case kVertexBuffer :
		bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		break;

	case kIndexBuffer :
		bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		break;

	case kConstantBuffer :
		bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		break;

	case kShaderResource :
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		break;

	default:
		return false;
	}

	VkMemoryPropertyFlags properties = 0;
	switch(usage)
	{
	case kBufferUsageDefault :
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	
	case kBufferUsageDynamic :
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	
	case kBufferUsageImmutable :
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	
	case kBufferUsageStaging :
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		break;
	
	default:
		return false;
	}

	VkDevice device = VulkanClient::instance ().getLogicalDevice ();
	
	if(vkCreateBuffer (device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		return false;
	ASSERT (buffer != VK_NULL_HANDLE)
	
	VkMemoryRequirements memRequirements {};
	vkGetBufferMemoryRequirements (device, buffer, &memRequirements);
	
	VkPhysicalDeviceMemoryProperties memProperties {};
	vkGetPhysicalDeviceMemoryProperties (VulkanClient::instance ().getPhysicalDevice (), &memProperties);

	int selectedMemoryType = -1;
	for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if(get_flag<uint32_t> (memRequirements.memoryTypeBits, (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			selectedMemoryType = i;
			break;
		}
	}
	if(selectedMemoryType < 0)
		return false;
	
	VkMemoryAllocateInfo allocInfo {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = selectedMemoryType;

	if(vkAllocateMemory (device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
		return false;
	
	if(vkBindBufferMemory (device, buffer, memory, 0) != VK_SUCCESS)
		return false;
	
	if(initialData)
	{
		void* dst = map ();
		if(dst)
			::memcpy (dst, initialData, sizeInBytes);
		unmap ();
	}
	
	capacity = sizeInBytes;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* Vulkan3DBuffer::map ()
{
	ASSERT (memory != VK_NULL_HANDLE)
	if(memory == VK_NULL_HANDLE)
		return nullptr;

	if(mappedData == nullptr)
	{
		VkDevice device = VulkanClient::instance ().getLogicalDevice ();
		VkResult result = vkMapMemory (device, memory, 0, bufferInfo.size, 0, &mappedData);
		ASSERT (result == VK_SUCCESS)
	}
	mapCount++;
	return mappedData;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DBuffer::unmap ()
{
	mapCount--;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Vulkan3DBuffer::ensureSegmentAlignment (uint32& byteOffset, uint32& size, uint32 stride) const
{
	uint32 alignment = memoryAlignment;
	if(buffer != VK_NULL_HANDLE)
	{
		VkDevice device = VulkanClient::instance ().getLogicalDevice ();
		VkMemoryRequirements requirements {};
		vkGetBufferMemoryRequirements (device, buffer, &requirements);
		alignment = ccl_lowest_common_multiple<uint32> (uint32 (requirements.alignment), alignment);
	}
	alignment = ccl_lowest_common_multiple<uint32> (alignment, stride);

	byteOffset = ccl_align_to (byteOffset, alignment);
	size = ccl_align_to (size, alignment);
	
	return true;
}

//************************************************************************************************
// Vulkan3DTexture2D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Vulkan3DTexture2D, Native3DTexture2D)

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DTexture2D::Vulkan3DTexture2D ()
: rowSize (0),
  immutable (false),
  addressMode (VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Vulkan3DTexture2D::create (uint32 width, uint32 height, uint32 bytesPerRow, DataFormat3D format, TextureFlags3D flags, const void* initialData)
{
	rowSize = bytesPerRow;

	BufferUsage3D usage = kBufferUsageDynamic;
	if(get_flag<TextureFlags3D> (flags, kTextureImmutable))
	{
		immutable = true;
		usage = kBufferUsageImmutable;
	}

	uint32 mipLevels = 1;
	if(get_flag<TextureFlags3D> (flags, kTextureMipmapEnabled))
		mipLevels = getMipLevels (width, height);

	uint32 sizeInBytes = bytesPerRow * height;
	uint32 texelByteSize = getVulkanFormatSize (format);
	rowSize = bytesPerRow / texelByteSize;

	if(!stagingBuffer.create (IGraphicsBuffer3D::kShaderResource, usage, sizeInBytes, texelByteSize, initialData))
		return false;

	image.setUsage (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	image.setFormat (getVulkanFormat (format));
	image.setSize ({ width, height });
	image.setMipLevels (mipLevels);
	if(!image.create ())
		return false;

	if(initialData)
		upload ();

	addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	if(get_flag<TextureFlags3D> (flags, kTextureClampToBorder))
		addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	else if(get_flag<TextureFlags3D> (flags, kTextureRepeat))
		addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	else if(get_flag<TextureFlags3D> (flags, kTextureMirror))
		addressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DTexture2D::upload ()
{
	VulkanClient& client = VulkanClient::instance ();
	VkCommandBuffer commandBuffer = client.beginSingleTimeCommands ();
	if(commandBuffer == nullptr)
	{
		CCL_WARN ("%s\n", "Failed to allocate a command buffer for uploading a texture.")
		return;
	}
	
	image.transition (commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	
	VkBufferImageCopy region {};
	region.bufferOffset = 0;
	region.bufferRowLength = rowSize;
	region.bufferImageHeight = image.getSize ().height;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { image.getSize ().width, image.getSize ().height, 1 };
	
	vkCmdCopyBufferToImage (commandBuffer, stagingBuffer.getBuffer (), image.getImage (), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	
	bool transitioned = image.generateMipmaps (commandBuffer);
	if(!transitioned)
		image.transition (commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	client.endSingleTimeCommands (commandBuffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DTexture2D::copyFromBitmap (IBitmap* bitmap)
{
	if(immutable)
		return kResultFailed;

	if(image.getFormat () != VulkanImage::kNativeImageFormat)
		return kResultFailed;

	IMultiResolutionBitmap::RepSelector selector (UnknownPtr<IMultiResolutionBitmap> (bitmap), getHighestResolutionIndex (bitmap));
	BitmapDataLocker locker (bitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
	if(locker.result != kResultOk)
		return kResultFailed;

	if(locker.data.width != image.getSize ().width || locker.data.height != image.getSize ().height)
		return kResultInvalidArgument;

	if(stagingBuffer.getBuffer () == VK_NULL_HANDLE)
		return kResultFailed;

	void* stagingData = stagingBuffer.map ();
	if(stagingData == nullptr)
		return kResultFailed;

	::memcpy (stagingData, locker.data.scan0, locker.data.rowBytes * locker.data.height);

	stagingBuffer.unmap ();
	upload ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DTexture2D::destroy ()
{
	stagingBuffer.destroy ();
	image.destroy ();
}

//************************************************************************************************
// Vulkan3DShader
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Vulkan3DShader, Native3DGraphicsShader)

const FileType Vulkan3DShader::kFileType ("Compiled SPIR-V Shader Object", "spv");

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DShader::Vulkan3DShader ()
: shader (VK_NULL_HANDLE)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DShader::~Vulkan3DShader ()
{
	reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DShader::reset ()
{
	VkDevice device = VulkanClient::instance ().getLogicalDevice ();

	if(shader)
		vkDestroyShaderModule (device, shader, nullptr);
	shader = VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Vulkan3DShader::create (GraphicsShader3DType _type, UrlRef _path)
{
	reset ();

	path = _path;
	type = _type;
	
	return load ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Vulkan3DShader::load ()
{
	// load pre-compiled shader from a spir-v file
	AutoPtr<IMemoryStream> stream = File::loadBinaryFile (path);
	if(stream == nullptr)
		return false;
	
	VkShaderModuleCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = stream->getBytesWritten ();
	createInfo.pCode = static_cast<const uint32_t*> (stream->getMemoryAddress ());
	
	VkDevice device = VulkanClient::instance ().getLogicalDevice ();
	if(!device || vkCreateShaderModule (device, &createInfo, nullptr, &shader) != VK_SUCCESS)
		return false;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::ITypeInfo* CCL_API Vulkan3DShader::getBufferTypeInfo (int bufferIndex)
{
	if(bufferTypeInfos.isEmpty ())
		GLSLShaderReflection::getBufferTypeInfos (bufferTypeInfos, path);
	return SuperClass::getBufferTypeInfo (bufferIndex);
}

//************************************************************************************************
// Vulkan3DDescriptorSet
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Vulkan3DDescriptorSet, Native3DShaderParameterSet)

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DDescriptorSet::Vulkan3DDescriptorSet ()
: descriptorPool (VK_NULL_HANDLE),
  descriptorSet (VK_NULL_HANDLE)
{
	VkDevice device = VulkanClient::instance ().getLogicalDevice ();
	
	// create descriptor pool
	
	VkDescriptorPoolSize poolSize[2] = {{}};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 3; // UNSURE do we need one descriptor set for each swapchain image?
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = kMaxTextureCount; // UNSURE do we need one descriptor set for each swapchain image?

	VkDescriptorPoolCreateInfo poolInfo {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = ARRAY_COUNT (poolSize);
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = 1; // UNSURE do we need one descriptor set for each swapchain image?
	VkResult result = vkCreateDescriptorPool (device, &poolInfo, nullptr, &descriptorPool);
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s: %s\n", "Failed to create a descriptor pool", string_VkResult (result))
	}
	
	// create descriptor set
	
	VkDescriptorSetLayout layout = VulkanClient::instance ().getDescriptorSetLayout ();
	
	VkDescriptorSetAllocateInfo descriptorSetInfo {};
	descriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetInfo.descriptorPool = descriptorPool;
	descriptorSetInfo.descriptorSetCount = 1;
	descriptorSetInfo.pSetLayouts = &layout;
	result = vkAllocateDescriptorSets (device, &descriptorSetInfo, &descriptorSet);
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s: %s\n", "Failed to allocate descriptor set", string_VkResult (result))
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DDescriptorSet::~Vulkan3DDescriptorSet ()
{
	VkDevice device = VulkanClient::instance ().getLogicalDevice ();
	
	if(descriptorPool)
	{
		vkDeviceWaitIdle (device);
		vkDestroyDescriptorPool (device, descriptorPool, nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DDescriptorSet::updateDescriptorSet ()
{
	Vector<VkWriteDescriptorSet> descriptorSets (vertexShaderParameters.count () + pixelShaderParameters.count () + 1);
	Vector<VkDescriptorBufferInfo> shaderParamInfo (vertexShaderParameters.count () + pixelShaderParameters.count ());
	
	for(int i = 0; i < vertexShaderParameters.count () + pixelShaderParameters.count (); i++)
	{
		Native3DShaderParameters& parameters = i < vertexShaderParameters.count () ? vertexShaderParameters[i] : pixelShaderParameters[i - vertexShaderParameters.count ()];
		
		IBufferSegment3D* segment = parameters.segment;
		if(segment == nullptr)
			continue;
		
		auto* parameterBuffer = unknown_cast<Vulkan3DBuffer> (segment->getBuffer ());
		
		VkDescriptorBufferInfo info {};
		info.buffer = parameterBuffer->getBuffer ();
		info.offset = segment->getOffset ();
		info.range = segment->getSize () > 0 ? segment->getSize () : VK_WHOLE_SIZE;
		
		shaderParamInfo.add (info);
		
		VkWriteDescriptorSet vertexParamsDescriptorWrite {};
		vertexParamsDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vertexParamsDescriptorWrite.dstSet = descriptorSet;
		vertexParamsDescriptorWrite.dstBinding = parameters.bufferIndex;
		vertexParamsDescriptorWrite.dstArrayElement = 0;
		vertexParamsDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vertexParamsDescriptorWrite.descriptorCount = 1;
		vertexParamsDescriptorWrite.pBufferInfo = &shaderParamInfo.last ();
		
		descriptorSets.add (vertexParamsDescriptorWrite);
	}
	
	VkWriteDescriptorSet textureDescriptorWrite = {};
	VkDescriptorImageInfo textureImageInfo[kMaxTextureCount] = {};
	for(int i = 0; i < kMaxTextureCount; i++)
	{
		Vulkan3DTexture2D* texture = unknown_cast<Vulkan3DTexture2D> (textures[i]);
		
		textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		textureImageInfo[i].imageView = texture ? texture->getImage ().getImageView () : Vulkan3DResourceManager::instance ().getNullImage ().getImageView ();
		textureImageInfo[i].sampler = Vulkan3DResourceManager::instance ().getSampler (texture ? texture->getAddressMode () : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, i);
	}

	textureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	textureDescriptorWrite.dstSet = descriptorSet;
	textureDescriptorWrite.dstBinding = kLastShaderParameterIndex + 1;
	textureDescriptorWrite.dstArrayElement = 0;
	textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureDescriptorWrite.descriptorCount = kMaxTextureCount;
	textureDescriptorWrite.pImageInfo = textureImageInfo;
	
	descriptorSets.add (textureDescriptorWrite);

	vkUpdateDescriptorSets (VulkanClient::instance ().getLogicalDevice (), descriptorSets.count (), descriptorSets, 0, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DDescriptorSet::setVertexShaderParameters (int bufferIndex, IBufferSegment3D* parameters)
{
	Native3DShaderParameters* shaderParameters = findVertexShaderParameters (bufferIndex);
	if((shaderParameters == nullptr || shaderParameters->segment == nullptr) && parameters == nullptr)
		return kResultOk;
	
	if(shaderParameters == nullptr || shaderParameters->segment == nullptr || (parameters != nullptr && !shaderParameters->segment->isEqual (*parameters)))
	{
		Native3DShaderParameterSet::setVertexShaderParameters (bufferIndex, parameters);
		updateDescriptorSet ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DDescriptorSet::setPixelShaderParameters (int bufferIndex, IBufferSegment3D* parameters)
{
	Native3DShaderParameters* shaderParameters = findPixelShaderParameters (bufferIndex);
	if((shaderParameters == nullptr || shaderParameters->segment == nullptr) && parameters == nullptr)
		return kResultOk;
	
	if(shaderParameters == nullptr || shaderParameters->segment == nullptr || (parameters != nullptr && !shaderParameters->segment->isEqual (*parameters)))
	{
		Native3DShaderParameterSet::setPixelShaderParameters (bufferIndex, parameters);
		updateDescriptorSet ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DDescriptorSet::setTexture (int textureIndex, IGraphicsTexture2D* texture)
{
	if(texture != textures.at (textureIndex))
	{
		SuperClass::setTexture (textureIndex, texture);
		updateDescriptorSet ();
	}
	return kResultOk;
}

//************************************************************************************************
// Vulkan3DShaderManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Vulkan3DResourceManager, Native3DResourceManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DResourceManager::Vulkan3DResourceManager ()
{
	samplers.setCount (kNumAddressModes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DResourceManager::shutdown ()
{
	VulkanClient& client = VulkanClient::instance ();
	VkDevice device = client.getLogicalDevice ();
	
	for(int i = 0; i < samplers.count (); i++)
	{
		for(int j = 0; j < samplers[i].count (); j++)
			vkDestroySampler (device, samplers[i][j], nullptr);
		samplers[i].zeroFill ();
		samplers[i].setCount (0);
	}
	
	nullTexture.destroy ();

	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VkSampler Vulkan3DResourceManager::getSampler (VkSamplerAddressMode addressMode, int textureIndex) const
{
	ASSERT (addressMode < samplers.count ())
	if(addressMode >= samplers.count ())
		return VK_NULL_HANDLE;

	if(samplers[addressMode].isEmpty ())
	{
		VulkanClient& client = VulkanClient::instance ();
		VkDevice device = client.getLogicalDevice ();
		VkPhysicalDevice physicalDevice = client.getPhysicalDevice ();
		
		VkSamplerCreateInfo samplerInfo {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		
		VkPhysicalDeviceProperties properties {};
		vkGetPhysicalDeviceProperties (physicalDevice, &properties);
		samplerInfo.anisotropyEnable = client.getPhysicalDeviceFeatures ().samplerAnisotropy;
		samplerInfo.maxAnisotropy = samplerInfo.anisotropyEnable ? properties.limits.maxSamplerAnisotropy : 1.f;
		
		samplers[addressMode].setCount (Vulkan3DDescriptorSet::kMaxTextureCount);
		samplers[addressMode].zeroFill ();
		for(int i = 0; i < samplers[addressMode].count (); i++)
		{
			VkResult result = vkCreateSampler (device, &samplerInfo, nullptr, &samplers[addressMode][i]);
			if(result != VK_SUCCESS)
			{
				CCL_WARN ("%s: %s\n", "Failed to create an image sampler", string_VkResult (result))
			}
		}
	}
	return samplers[addressMode].at (textureIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsShader* Vulkan3DResourceManager::loadShader (UrlRef _path, GraphicsShader3DType type)
{
	AutoPtr<Vulkan3DShader> shader = NEW Vulkan3DShader;
	Url path (_path);
	path.setFileType (Vulkan3DShader::kFileType);
	if(!shader->create (type, path))
		return nullptr;
	
	return shader.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DTexture2D* Vulkan3DResourceManager::loadTexture (Bitmap* bitmap, TextureFlags3D flags)
{
	AutoPtr<Vulkan3DTexture2D> texture = NEW Vulkan3DTexture2D;
	if(texture->create (bitmap, flags))
		return texture.detach ();
	
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const VulkanImage& Vulkan3DResourceManager::getNullImage ()
{
	if(nullTexture.getImage ().getImage () == VK_NULL_HANDLE)
		nullTexture.create (1, 1, getVulkanFormatSize (kB8G8R8A8_UNORM), kB8G8R8A8_UNORM, kTextureImmutable, &Colors::kTransparentBlack);
	return nullTexture.getImage ();
}

//************************************************************************************************
// Vulkan3DPipeline
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Vulkan3DPipeline, Native3DGraphicsPipeline)

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DPipeline::Vulkan3DPipeline ()
: changed (true),
  pipelineLayout (VK_NULL_HANDLE),
  topology (VK_PRIMITIVE_TOPOLOGY_POINT_LIST),
  fillMode (VK_POLYGON_MODE_FILL),
  depthTestEnabled (true),
  depthWriteEnabled (true),
  depthBias (0.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DPipeline::~Vulkan3DPipeline ()
{
	reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DPipeline::reset ()
{
	VkDevice device = VulkanClient::instance ().getLogicalDevice ();
	
	for(PipelineItem& item : pipelines)
	{
		if(item.pipeline != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle (device);
			vkDestroyPipeline (device, item.pipeline, nullptr);
		}
	}
	pipelines.removeAll ();
	
	if(pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout (device, pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;
	
	changed = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DShader* Vulkan3DPipeline::getShader (int index) const
{
	return shaderList.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DPipeline::setVertexShader (IGraphicsShader3D* shader)
{
	if(shader == nullptr || shader->getType () != IGraphicsShader3D::kVertexShader)
		return kResultInvalidArgument;
	
	return setShader (kVertexShaderIndex, shader);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DPipeline::setPixelShader (IGraphicsShader3D* shader)
{
	if(shader == nullptr || shader->getType () != IGraphicsShader3D::kPixelShader)
		return kResultInvalidArgument;
	
	return setShader (kPixelShaderIndex, shader);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DPipeline::setDepthTestParameters (const DepthTestParameters3D& parameters)
{
	if(parameters.testEnabled != depthTestEnabled || parameters.writeEnabled != depthWriteEnabled || parameters.bias != depthBias)
		changed = true;

	depthTestEnabled = parameters.testEnabled;
	depthWriteEnabled = parameters.writeEnabled;
	depthBias = parameters.bias;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Vulkan3DPipeline::setShader (int index, IGraphicsShader3D* shader)
{
	if(shader == nullptr && index < shaderList.count ())
	{
		shaderList[index].release ();
		changed = true;
		return kResultOk;
	}
	
	Vulkan3DShader* newShader = unknown_cast<Vulkan3DShader> (shader);
	if(newShader == nullptr)
		return kResultInvalidArgument;
	
	if(index >= shaderList.count ())
	{
		shaderList.setCount (index + 1);
		changed = true;
	}
	
	if(shaderList[index] != newShader)
		changed = true;
	shaderList[index] = newShader;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DPipeline::setVertexFormat (IVertexFormat3D* _format)
{
	Vulkan3DVertexFormat* format = unknown_cast<Vulkan3DVertexFormat> (_format);
	if(format == nullptr)
		return kResultInvalidArgument;

	if(format != vertexFormat)
		changed = true;
	vertexFormat = format;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DPipeline::setPrimitiveTopology (PrimitiveTopology3D _primitiveTopology)
{
	VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	switch(_primitiveTopology)
	{
	case kPrimitiveTopologyTriangleList : 
		primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		break;
	case kPrimitiveTopologyTriangleStrip : 
		primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		break;
	case kPrimitiveTopologyTriangleFan : 
		primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		break;
	default :
		return kResultInvalidArgument;
	}
	
	if(topology != primitiveTopology)
		changed = true;
	topology = primitiveTopology;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DPipeline::setFillMode (FillMode3D mode)
{
	VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
	switch(mode)
	{
	case kFillModeSolid : 
		polygonMode = VK_POLYGON_MODE_FILL; 
		break;
	case kFillModeWireframe : 
		polygonMode = VK_POLYGON_MODE_LINE; 
		break;
	default : 
		return kResultInvalidArgument;
	}
	
	if(fillMode != polygonMode)
		changed = true;
	fillMode = polygonMode;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VkPipeline Vulkan3DPipeline::getPipeline (VkRenderPass renderpass, VkSampleCountFlagBits sampleCount)
{
	if(renderpass == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;
	
	if(changed)
		reset ();
	changed = false;
	
	for(const PipelineItem& item : pipelines)
	{
		if(item.renderpass == renderpass && item.sampleCount == sampleCount && item.pipeline != VK_NULL_HANDLE)
			return item.pipeline;
	}
	
	return createPipeline (renderpass, sampleCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VkPipeline Vulkan3DPipeline::createPipeline (VkRenderPass renderpass, VkSampleCountFlagBits sampleCount)
{
	VkDevice device = VulkanClient::instance ().getLogicalDevice ();
	
	if(vertexFormat == nullptr)
		return VK_NULL_HANDLE;
	
	if(pipelineLayout == VK_NULL_HANDLE)
	{
		VkDescriptorSetLayout layouts[] = { VulkanClient::instance ().getDescriptorSetLayout () };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = layouts;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		VkResult result = vkCreatePipelineLayout (device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
		ASSERT (result == VK_SUCCESS)
		if(result != VK_SUCCESS)
			return VK_NULL_HANDLE;
	}
	
	Vector<VkPipelineShaderStageCreateInfo> shaderInfo;
	for(int i = 0; i < shaderList.count (); i++)
	{
		Vulkan3DShader* shader = shaderList.at (i);
		if(shader == nullptr)
			continue;
		
		VkPipelineShaderStageCreateInfo info {};
		
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		
		if(shader->getType () == IGraphicsShader3D::kVertexShader)
			info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		else if(shader->getType () == IGraphicsShader3D::kPixelShader)
			info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		else 
			continue;
			
		info.module = shader->getShader ();
		info.pName = CSTR ("main");
		
		shaderInfo.add (info);
	}
	
	VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicState {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = ARRAY_COUNT (dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;
	
	VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = topology;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	
	VkViewport viewportDescription {};
	viewportDescription.x = 0;
	viewportDescription.y = 0;
	viewportDescription.width = 0;
	viewportDescription.height = 0;
	viewportDescription.minDepth = 0.0f;
	viewportDescription.maxDepth = 1.0f;
	
	VkRect2D scissor {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = 0;
	scissor.extent.height = 0;

	VkPipelineViewportStateCreateInfo viewportState {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewportDescription;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
	
	VkPipelineRasterizationStateCreateInfo rasterizer {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = fillMode;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = depthBias != 0.0f ? VK_TRUE : VK_FALSE;
	rasterizer.depthBiasConstantFactor = depthBias;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = sampleCount > 1 ? VK_TRUE : VK_FALSE;
	multisampling.rasterizationSamples = sampleCount;
	multisampling.minSampleShading = 0.2f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE; // sampleCount > 1 ? VK_TRUE : VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	
	VkPipelineColorBlendStateCreateInfo colorBlending {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencil {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = depthTestEnabled ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = depthWriteEnabled ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};
	
	VkGraphicsPipelineCreateInfo pipelineInfo {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderInfo.count ();
	pipelineInfo.pStages = shaderInfo;
	pipelineInfo.pVertexInputState = &vertexFormat->getVertexInputInfo ();
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderpass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	
	VkPipelineCache pipelineCache = VulkanClient::instance ().getPipelineCache ();
	VkPipeline pipeline;
	VkResult result = vkCreateGraphicsPipelines (device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline);
	ASSERT (result == VK_SUCCESS)
	if(result == VK_SUCCESS)
	{
		pipelines.add ({ renderpass, sampleCount, pipeline });
		return pipeline;
	}
	
	return VK_NULL_HANDLE;
}

//************************************************************************************************
// Vulkan3DGraphicsFactory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Vulkan3DGraphicsFactory, Native3DGraphicsFactory)

//////////////////////////////////////////////////////////////////////////////////////////////////

IVertexFormat3D* CCL_API Vulkan3DGraphicsFactory::createVertexFormat (const VertexElementDescription description[], uint32 count, const IGraphicsShader3D* shader)
{
	AutoPtr<Vulkan3DVertexFormat> format = NEW Vulkan3DVertexFormat;
	if(!format->create (description, count, shader))
		return nullptr;

	return format.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsBuffer3D* CCL_API Vulkan3DGraphicsFactory::createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage,
								uint32 sizeInBytes, uint32 strideInBytes, const void* initialData /* = nullptr */)
{
	AutoPtr<Vulkan3DBuffer> buffer = NEW Vulkan3DBuffer;
	if(!buffer->create (type, usage, sizeInBytes, strideInBytes, initialData))
		return nullptr;

	return buffer.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsTexture2D* CCL_API Vulkan3DGraphicsFactory::createTexture (IBitmap* _bitmap, TextureFlags3D flags)
{
	auto* bitmap = unknown_cast<Bitmap> (_bitmap);
	if(bitmap == nullptr)
		return nullptr;

	if(get_flag<TextureFlags3D> (flags, kTextureImmutable))
	{
		Vulkan3DResourceManager& manager = Vulkan3DResourceManager::instance ();
		return return_shared (manager.getTexture (bitmap, flags));
	}

	AutoPtr<Vulkan3DTexture2D> texture = NEW Vulkan3DTexture2D;
	if(!texture->create (bitmap, flags))
		return nullptr;

	return texture.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsShader3D* CCL_API Vulkan3DGraphicsFactory::createShader (GraphicsShader3DType type, UrlRef path)
{
	Vulkan3DResourceManager& manager = Vulkan3DResourceManager::instance ();
	return return_shared (manager.getShader (path, type));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsShader3D* CCL_API Vulkan3DGraphicsFactory::createStockShader (GraphicsShader3DType type, StringID name)
{
	ResourceUrl url {String (name)};
	Vulkan3DResourceManager& manager = Vulkan3DResourceManager::instance ();
	return return_shared (manager.getShader (url, type));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsPipeline3D* CCL_API Vulkan3DGraphicsFactory::createPipeline ()
{
	return NEW Vulkan3DPipeline;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IShaderParameterSet3D* CCL_API Vulkan3DGraphicsFactory::createShaderParameterSet ()
{
	return NEW Vulkan3DDescriptorSet;
}

//************************************************************************************************
// Vulkan3DGraphicsContext
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Vulkan3DGraphicsContext, Native3DGraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

Vulkan3DGraphicsContext::Vulkan3DGraphicsContext ()
: renderpass (VK_NULL_HANDLE),
  bufferStride (0),
  indexBufferFormat (kR32_UInt),
  commandBuffer (nullptr),
  sampleCount (VK_SAMPLE_COUNT_1_BIT)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DGraphicsContext::setPipeline (IGraphicsPipeline3D* _graphicsPipeline)
{
	Vulkan3DPipeline* graphicsPipeline = unknown_cast<Vulkan3DPipeline> (_graphicsPipeline);
	if(graphicsPipeline == nullptr)
		return kResultInvalidArgument;
	
	pipeline = graphicsPipeline;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DGraphicsContext::setVertexBuffer (IGraphicsBuffer3D* _buffer, uint32 stride)
{
	Vulkan3DBuffer* buffer = unknown_cast<Vulkan3DBuffer> (_buffer);
	if(buffer == nullptr)
		return kResultInvalidArgument;

	vertexBuffer = buffer;
	bufferStride = stride;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DGraphicsContext::setIndexBuffer (IGraphicsBuffer3D* _buffer, DataFormat3D format)
{
	Vulkan3DBuffer* buffer = unknown_cast<Vulkan3DBuffer> (_buffer);
	if(buffer == nullptr)
		return kResultInvalidArgument;

	indexBuffer = buffer;
	indexBufferFormat = format;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DGraphicsContext::setShaderParameters (IShaderParameterSet3D* _parameters)
{
	Vulkan3DDescriptorSet* parameters = unknown_cast<Vulkan3DDescriptorSet> (_parameters);
	if(parameters == nullptr)
		return kResultInvalidArgument;

	shaderParameters = parameters;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Vulkan3DGraphicsContext::bindPipeline ()
{
	if(!pipeline.isValid ())
		return false;

	VkPipeline vulkanPipeline = pipeline->getPipeline (renderpass, sampleCount);
	if(!vulkanPipeline)
		return false;

	vkCmdBindPipeline (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Vulkan3DGraphicsContext::bindDescriptorSet ()
{
	if(shaderParameters.isValid () && pipeline.isValid ())
	{
		VkDescriptorSet descriptorSets[] = { shaderParameters->getDescriptorSet () };
		if(descriptorSets[0])
			vkCmdBindDescriptorSets (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout (), 0, ARRAY_COUNT (descriptorSets), descriptorSets, 0, nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Vulkan3DGraphicsContext::prepareDrawing ()
{
	if(viewport.isEmpty ())
		return kResultOk;

	if(vertexBuffer == nullptr)
		return kResultFailed;

	if(!bindPipeline ())
		return kResultFailed;

	VkBuffer vertexBuffers[] = { vertexBuffer->getBuffer () };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers (commandBuffer, 0, 1, vertexBuffers, offsets);

	VkViewport viewportDescription {};
	viewportDescription.x = viewport.left;
	viewportDescription.y = viewport.top;
	viewportDescription.width = viewport.getWidth ();
	viewportDescription.height = viewport.getHeight ();
	viewportDescription.minDepth = 0.0f;
	viewportDescription.maxDepth = 1.0f;
	vkCmdSetViewport (commandBuffer, 0, 1, &viewportDescription);

	VkRect2D scissor {};
	scissor.offset.x = viewport.left;
	scissor.offset.y = viewport.top;
	scissor.extent.width = viewport.getWidth ();
	scissor.extent.height = viewport.getHeight ();
	vkCmdSetScissor (commandBuffer, 0, 1, &scissor);
	
	bindDescriptorSet ();
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DGraphicsContext::draw (uint32 startVertex, uint32 vertexCount)
{
	tresult result = prepareDrawing ();
	if(result != kResultOk)
		return result;

	vkCmdDraw (commandBuffer, vertexCount, 1, startVertex, 0);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Vulkan3DGraphicsContext::drawIndexed (uint32 startIndex, uint32 indexCount, int32 baseVertex)
{
	tresult result = prepareDrawing ();
	if(result != kResultOk)
		return result;

	ASSERT (indexBufferFormat == kR16_UInt || indexBufferFormat == kR32_UInt)
	vkCmdBindIndexBuffer (commandBuffer, indexBuffer->getBuffer (), 0, indexBufferFormat == kR16_UInt ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed (commandBuffer, indexCount, 1, startIndex, baseVertex, 0);

	return kResultOk;
}

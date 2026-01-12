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
// Filename    : ccl/platform/shared/vulkan/vulkanclient.h
// Description : Vulkan Client Context
//
//************************************************************************************************

#ifndef _vulkanclient_h
#define _vulkanclient_h

#include "ccl/base/singleton.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/text/cstring.h"

#include <vulkan/vulkan.h>

#if !CCL_PLATFORM_ANDROID
#include "ccl/platform/shared/skia/skiaglue.h"
#endif

#define ENABLE_VALIDATION_LAYERS (1 && DEBUG && !CCL_PLATFORM_ANDROID)
#define ENABLE_EXTENDED_VALIDATION (0 && ENABLE_VALIDATION_LAYERS)

namespace CCL {

//************************************************************************************************
// VulkanGPUContext
//************************************************************************************************

#if CCL_PLATFORM_ANDROID
typedef void VulkanGPUContext;
#else
typedef GrRecordingContext VulkanGPUContext;
#endif

//************************************************************************************************
// VulkanClient
//************************************************************************************************

class VulkanClient: public Object,
					public ExternalSingleton<VulkanClient>
{
public:
	VulkanClient ();
	~VulkanClient ();
	
	virtual void initialize (VkSurfaceKHR surface);
	virtual bool initializeLogicalDevice ();
	virtual void terminate ();

	bool isInitialized () const { return initialized; }
	
	static const int kAPIVersion;
	
	bool isSupported ();
	VkPhysicalDeviceType getDeviceType () const { return deviceType; }
	VkInstance getVulkanInstance () const { return vulkanInstance; }
	VkPhysicalDevice getPhysicalDevice () const { return physicalDevice; }
	const VkPhysicalDeviceFeatures& getPhysicalDeviceFeatures () const { return physicalDeviceFeatures; }
	VkDevice getLogicalDevice () const { return logicalDevice; }
	VkQueue getGraphicsQueue () const { return graphicsQueue; }
	uint32_t getGraphicsQueueFamilyIndex () const { return graphicsQueueFamilyIndex; }
	VkQueue getPresentationQueue () const { return presentationQueue; }
	uint32_t getPresentationQueueFamilyIndex () const { return presentationQueueFamilyIndex; }
	const Vector<CStringPtr>& getVulkanExtensions () const { return vulkanExtensions; }
	const Vector<CStringPtr>& getDeviceExtensions () const { return deviceExtensions; }
	VkCommandPool getCommandPool () const { return commandPool; }
	VkDescriptorSetLayout getDescriptorSetLayout () const { return descriptorSetLayout; }
	VkPipelineCache getPipelineCache () const { return pipelineCache; }

	VkCommandBuffer beginSingleTimeCommands ();
	void endSingleTimeCommands (VkCommandBuffer commandBuffer);

	virtual VulkanGPUContext* getGPUContext () { return nullptr; }
	
protected:
	static const Vector<CStringPtr> kRequiredVulkanExtensions;
	static const Vector<CStringPtr> kOptionalVulkanExtensions;
	static const Vector<CStringPtr> kRequiredDeviceExtensions;
	static const Vector<CStringPtr> kOptionalDeviceExtensions;
	static const Vector<CStringPtr> kValidationLayers;
	
	bool initialized;

	VkInstance vulkanInstance;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceType deviceType;
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	MutableCString deviceName;
	VkDevice logicalDevice;
	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamilyIndex;
	VkQueue presentationQueue;
	uint32_t presentationQueueFamilyIndex;
	VkCommandPool commandPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineCache pipelineCache;
	
	Vector<VkExtensionProperties> extensionProperties;
	Vector<CStringPtr> vulkanExtensions;
	Vector<CStringPtr> deviceExtensions;
	Vector<bool> selectedExtensions;
	Vector<VkPhysicalDevice> deviceCandidates;
	
	#if ENABLE_VALIDATION_LAYERS
	VkDebugUtilsMessengerEXT debugMessenger;
	#endif
	
	virtual bool initializePlatform () = 0;
	virtual const Vector<CStringPtr>& getRequiredPlatformExtensions () const = 0;
	
	void createVulkanInstance ();
};

} // namespace CCL

#endif // _vulkanclient_h

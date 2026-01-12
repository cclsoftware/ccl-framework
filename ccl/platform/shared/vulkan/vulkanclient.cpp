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
// Filename    : ccl/platform/shared/vulkan/vulkanclient.cpp
// Description : Vulkan Client Context
//
//************************************************************************************************

#include "ccl/platform/shared/vulkan/vulkanclient.h"
#include "ccl/platform/shared/vulkan/vulkan3dsupport.h"

#include "ccl/public/cclversion.h"
#include "ccl/public/gui/graphics/3d/stockshader3d.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

using namespace CCL;

//************************************************************************************************
// VulkanClient
//************************************************************************************************

const Vector<CStringPtr> VulkanClient::kRequiredVulkanExtensions = 
{
	VK_KHR_SURFACE_EXTENSION_NAME
};
const Vector<CStringPtr> VulkanClient::kOptionalVulkanExtensions = 
{
	#if ENABLE_VALIDATION_LAYERS
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	#endif
	#if ENABLE_EXTENDED_VALIDATION
	, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME
	#endif
};
const Vector<CStringPtr> VulkanClient::kRequiredDeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const Vector<CStringPtr> VulkanClient::kOptionalDeviceExtensions =
{
	VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME
};

const Vector<CStringPtr> VulkanClient::kValidationLayers = { "VK_LAYER_KHRONOS_validation" };

#if CCL_PLATFORM_ANDROID
const int VulkanClient::kAPIVersion = VK_API_VERSION_1_0;
#else
const int VulkanClient::kAPIVersion = VK_API_VERSION_1_1;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

#if ENABLE_VALIDATION_LAYERS
static VKAPI_ATTR VkBool32 VKAPI_CALL validationLayerCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
	if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		CCL_WARN ("Vulkan: %s\n", pCallbackData->pMessage);	
	return VK_FALSE;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

VulkanClient::VulkanClient ()
: initialized (false),
  vulkanInstance (nullptr),
  physicalDevice (nullptr),
  deviceType (VK_PHYSICAL_DEVICE_TYPE_OTHER),
  logicalDevice (nullptr),
  graphicsQueue (nullptr),
  presentationQueue (nullptr),
  graphicsQueueFamilyIndex (VK_QUEUE_FAMILY_IGNORED),
  presentationQueueFamilyIndex (VK_QUEUE_FAMILY_IGNORED),
  commandPool (VK_NULL_HANDLE),
  descriptorSetLayout (VK_NULL_HANDLE),
  pipelineCache (VK_NULL_HANDLE)
#if ENABLE_VALIDATION_LAYERS
  , debugMessenger (nullptr)
#endif
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

VulkanClient::~VulkanClient ()
{
	terminate ();
	
#if ENABLE_VALIDATION_LAYERS
	auto destroyDebugMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT> (vkGetInstanceProcAddr (vulkanInstance, "vkDestroyDebugUtilsMessengerEXT"));
	if(destroyDebugMessenger)
		destroyDebugMessenger (vulkanInstance, debugMessenger, nullptr);
#endif
	if(vulkanInstance != nullptr)
		vkDestroyInstance (vulkanInstance, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanClient::createVulkanInstance ()
{
	// Create vulkan instance
	
	VkApplicationInfo appInfo {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	//appInfo.pApplicationName = "";
	//appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = CCL_PRODUCT_NAME;
	appInfo.engineVersion = VK_MAKE_VERSION (CCL_VERSION_MAJOR, CCL_VERSION_MINOR, CCL_VERSION_REVISION);
	appInfo.apiVersion = kAPIVersion;
	
	VkInstanceCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	
	Vector<CStringPtr> requiredExtensions;
	for(CStringPtr extension : kRequiredVulkanExtensions)
		requiredExtensions.add (extension);
	for(CStringPtr extension : getRequiredPlatformExtensions ())
		requiredExtensions.add (extension);
	
	Vector<bool> availableExtensions;
	availableExtensions.setCount (requiredExtensions.count () + kOptionalVulkanExtensions.count ());
	availableExtensions.zeroFill ();

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, nullptr);
	Vector<VkExtensionProperties> extensions;
	extensions.setCount (extensionCount);
	vkEnumerateInstanceExtensionProperties (nullptr, &extensionCount, extensions);
	for(const VkExtensionProperties& extension : extensions)
	{
		CString extensionName (extension.extensionName);
		for(int i = 0; i < requiredExtensions.count (); i++)
		{
			if(extensionName == requiredExtensions[i])
			{
				vulkanExtensions.add (requiredExtensions[i]);
				availableExtensions[i] = true;
				break;
			}
		}
		for(int i = 0; i < kOptionalVulkanExtensions.count (); i++)
		{
			if(extensionName == kOptionalVulkanExtensions[i])
			{
				availableExtensions[i + requiredExtensions.count ()] = true;
				break;
			}
		}
	}

	bool extensionMissing = false;
	for(int i = 0; i < availableExtensions.count (); i++)
	{
		if(availableExtensions[i] == false)
		{
			if(i < requiredExtensions.count ())
				CCL_WARN ("Required Vulkan extension missing: %s\n", requiredExtensions[i])
			else
				CCL_WARN ("Optional Vulkan extension missing: %s\n", kOptionalVulkanExtensions[i - requiredExtensions.count ()])
		}
	}
	if(extensionMissing)
		return;

	bool debugUtilsAvailable = false;
	bool validationFeaturesAvailable = false;

	Vector<CStringPtr> vulkanExtensions;
	for(int i = 0; i < availableExtensions.count (); i++)
	{
		if(availableExtensions[i])
		{
			if(i < requiredExtensions.count ())
				vulkanExtensions.add (requiredExtensions[i]);
			else
			{
				CStringPtr extension = kOptionalVulkanExtensions[i - requiredExtensions.count ()];
				vulkanExtensions.add (extension);
				#if ENABLE_VALIDATION_LAYERS
				if(CString (extension) == VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
					debugUtilsAvailable = true;
				#endif
				#if ENABLE_EXTENDED_VALIDATION
				if(CString (extension) == VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME)
					validationFeaturesAvailable = true;
				#endif
			}
		}
	}
	createInfo.enabledExtensionCount = vulkanExtensions.count ();
	createInfo.ppEnabledExtensionNames = vulkanExtensions;

	#if ENABLE_VALIDATION_LAYERS
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties (&layerCount, nullptr);
	Vector<VkLayerProperties> availableLayers;
	availableLayers.setCount (layerCount);
	vkEnumerateInstanceLayerProperties (&layerCount, availableLayers);
	
	bool validationLayersAvailable = true;
	for(CStringPtr validationLayer : kValidationLayers)
	{
		bool layerAvailable = false;
		for(const VkLayerProperties& properties : availableLayers)
		{
			if(CString (properties.layerName) == validationLayer)
			{
				layerAvailable = true;
				break;
			}
		}
		if(!layerAvailable)
		{
			CCL_WARN ("%s\n", "Validation layers are not available")
			validationLayersAvailable = false;
			break;
		}
	}

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
	#if ENABLE_EXTENDED_VALIDATION
	VkValidationFeaturesEXT features = {};
	VkValidationFeatureEnableEXT validationFeatures[] = 
	{
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, 
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT, 
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
	};
	#endif
	if(validationLayersAvailable && debugUtilsAvailable)
	{
		createInfo.enabledLayerCount = kValidationLayers.count ();
		createInfo.ppEnabledLayerNames = kValidationLayers;
		createInfo.pNext = &debugCreateInfo;

		#if ENABLE_EXTENDED_VALIDATION
		if(validationFeaturesAvailable)
		{
			features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
			features.enabledValidationFeatureCount = ARRAY_COUNT (validationFeatures);
			features.pEnabledValidationFeatures = validationFeatures;
			
			createInfo.pNext = &features;
			features.pNext = &debugCreateInfo;
		}
		#endif // ENABLE_EXTENDED_VALIDATION
		
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = validationLayerCallback;
		debugCreateInfo.pUserData = nullptr;
	}
	#endif // ENABLE_VALIDATION_LAYERS
	
	VkResult result = vkCreateInstance (&createInfo, nullptr, &vulkanInstance);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS || vulkanInstance == nullptr)
	{
		CCL_WARN ("%s %s\n", "Failed to create a Vulkan instance!", string_VkResult (result))
		return;
	}
	
	#if ENABLE_VALIDATION_LAYERS
	if(validationLayersAvailable && debugUtilsAvailable)
	{
		auto createDebugMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT> (vkGetInstanceProcAddr (vulkanInstance, "vkCreateDebugUtilsMessengerEXT"));
		if(createDebugMessenger)
			createDebugMessenger (vulkanInstance, &debugCreateInfo, nullptr, &debugMessenger);
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanClient::initialize (VkSurfaceKHR surface)
{
	if(vulkanInstance == nullptr)
		createVulkanInstance ();
	if(vulkanInstance == nullptr)
		return;
	
	// Enumerate physical devices
	
	if(deviceCandidates.isEmpty ())
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices (vulkanInstance, &deviceCount, nullptr);
		ASSERT (deviceCount > 0)
		deviceCandidates.setCount (deviceCount);
		vkEnumeratePhysicalDevices (vulkanInstance, &deviceCount, deviceCandidates);
	}
	
	selectedExtensions.removeAll ();
	
	int bestScore = -1;
	for(const VkPhysicalDevice& device : deviceCandidates)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties (device, &deviceProperties);
		
		// check if required extensions are available
		
		Vector<bool> availableExtensions;
		availableExtensions.setCount (kRequiredDeviceExtensions.count () + kOptionalDeviceExtensions.count ()); 
		availableExtensions.zeroFill ();

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties (device, nullptr, &extensionCount, nullptr);
		Vector<VkExtensionProperties> extensions;
		extensions.setCount (extensionCount);
		vkEnumerateDeviceExtensionProperties (device, nullptr, &extensionCount, extensions);
		for(const VkExtensionProperties& extension : extensions)
		{
			CCL_PRINTF ("Device extension: %s\n", extension.extensionName);
			CString extensionName (extension.extensionName);
			for(int i = 0; i < kRequiredDeviceExtensions.count (); i++)
			{
				if(extensionName == kRequiredDeviceExtensions[i])
				{
					availableExtensions[i] = true;
					break;
				}
			}
			for(int i = 0; i < kOptionalDeviceExtensions.count (); i++)
			{
				if(extensionName == kOptionalDeviceExtensions[i])
				{
					availableExtensions[i + kRequiredDeviceExtensions.count ()] = true;
					break;
				}
			}
		}
		bool extensionsAvailable = true;
		for(int i = 0; i < kRequiredDeviceExtensions.count (); i++)
		{
			if(availableExtensions[i] == false)
			{
				CCL_WARN ("Required extension %s missing for device: %s\n", kRequiredDeviceExtensions[i], deviceProperties.deviceName)
				extensionsAvailable = false;
				break;
			}
		}
		if(!extensionsAvailable)
			continue;
		
		// calculate a score based on additional properties and features
		
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures (device, &deviceFeatures);
		
		int score = 0;
		if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 100;
		else if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
			score += 50;
		else if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
			score += 1;
		
		for(int i = 0; i < kOptionalDeviceExtensions.count (); i++)
		{
			if(availableExtensions[kRequiredDeviceExtensions.count () + i] == true)
				score += 2;
		}

		if(deviceFeatures.geometryShader)
			score += 1;
		if(deviceFeatures.dualSrcBlend)
			score += 1;
		if(deviceFeatures.sampleRateShading)
			score += 1;
		if(deviceFeatures.samplerAnisotropy)
			score += 1;

		// skip image format check if no surface is available, which is the case during initialization on Android;
		// the VK_GOOGLE_surfaceless_query extension to allow this without a surface was only added in Android 13
		if(surface != VK_NULL_HANDLE)
		{
			// check for image format support, fails for lavapipe
			Vector<VkSurfaceFormatKHR> formats;
			VkSurfaceFormatKHR format{};
			format.format = VK_FORMAT_UNDEFINED;
			uint32_t formatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &formatCount, nullptr);
			formats.setCount (formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &formatCount, formats);
			for(const VkSurfaceFormatKHR& availableFormat : formats)
			{
				format = availableFormat;
				if(availableFormat.format == VulkanImage::kNativeImageFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					break;
			}

			if(format.format == VK_FORMAT_UNDEFINED)
			{
				CCL_WARN ("No matching color format found for device: %s\n", deviceProperties.deviceName)
					score = -1;
			}
		}

		if(score > bestScore)
		{
			CCL_PRINTF ("Select device: %s\n", deviceProperties.deviceName)
			deviceName = deviceProperties.deviceName;
			deviceType = deviceProperties.deviceType;
			bestScore = score;
			physicalDevice = device;
			selectedExtensions = availableExtensions;
		}
	}
	if(physicalDevice == nullptr)
	{
		CCL_WARN ("%s\n", "Could not find a valid graphics device!")
		deviceCandidates.removeAll ();
		terminate ();
		return;
	}
	
	Debugger::printf ("Selected graphics device: %s\n", deviceName.str ());
	
	vkGetPhysicalDeviceFeatures (physicalDevice, &physicalDeviceFeatures);
	
	// Enumerate device queues
	
	presentationQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	graphicsQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &familyCount, nullptr);
	Vector<VkQueueFamilyProperties> queueFamilies;
	queueFamilies.setCount (familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &familyCount, queueFamilies);
	for(int i = 0; i < queueFamilies.count () && (presentationQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED || graphicsQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED); i++)
	{
		// skip presentation support check if no surface is available, which is the case during initialization on Android
		if(surface != VK_NULL_HANDLE)
		{
			VkBool32 presentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR (physicalDevice, i, surface, &presentationSupport);
			if(presentationSupport)
				presentationQueueFamilyIndex = i;
		}
		
		if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && (presentationQueueFamilyIndex == i || presentationQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED))
			graphicsQueueFamilyIndex = i;
	}
	if(presentationQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED && graphicsQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED)
		presentationQueueFamilyIndex = graphicsQueueFamilyIndex;
	if(graphicsQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED || presentationQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED)
	{
		CCL_WARN ("%s\n", "Could not find matching queue families.")
		deviceCandidates.remove (physicalDevice);
		terminate ();
		return;
	}
	
	initialized = initializeLogicalDevice ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanClient::initializeLogicalDevice ()
{
	if(logicalDevice != nullptr)
		vkDestroyDevice (logicalDevice, nullptr);
	logicalDevice = nullptr;
	
	// Create logical device
	
	VkDeviceQueueCreateInfo queueCreateInfo[2] {};
	queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamilyIndex;
	queueCreateInfo[0].queueCount = 1;
	float queuePriority = 1.f;
	queueCreateInfo[0].pQueuePriorities = &queuePriority;
	
	queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[1].queueFamilyIndex = presentationQueueFamilyIndex;
	queueCreateInfo[1].queueCount = 1;
	queueCreateInfo[1].pQueuePriorities = &queuePriority;
	
	deviceExtensions.removeAll ();
	for(int i = 0; i < selectedExtensions.count (); i++)
	{
		if(selectedExtensions[i])
		{
			if(i < kRequiredDeviceExtensions.count ())
				deviceExtensions.add (kRequiredDeviceExtensions[i]);
			else
				deviceExtensions.add (kOptionalDeviceExtensions[i - kRequiredDeviceExtensions.count ()]);
		}
	}
	
	VkDeviceCreateInfo deviceCreateInfo {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = graphicsQueueFamilyIndex == presentationQueueFamilyIndex ? 1 : 2;
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
	deviceCreateInfo.enabledExtensionCount = deviceExtensions.count ();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
	
	VkResult result = vkCreateDevice (physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s: %s, %s\n", "Failed to create a logical device instance", deviceName.str (), string_VkResult (result))
		CCL_WARN ("%s:\n", "Selected extensions:")
		for(int i = 0; i < deviceExtensions.count (); i++)
		{
			CCL_WARN ("\t%s\n", deviceExtensions[i])
		}
		deviceCandidates.remove (physicalDevice);
		terminate ();
		return false;
	}
	
	// Get graphics queue

	vkGetDeviceQueue (logicalDevice, graphicsQueueFamilyIndex, 0, &graphicsQueue);
	ASSERT (graphicsQueue != nullptr)
	if(graphicsQueue == nullptr)
	{
		CCL_WARN ("Could not get a graphics queue from %s\n", deviceName.str ())
		deviceCandidates.remove (physicalDevice);
		terminate ();
		return false;
	}
	
	// Get presentation queue

	vkGetDeviceQueue (logicalDevice, presentationQueueFamilyIndex, 0, &presentationQueue);
	ASSERT (presentationQueue != nullptr)
	if(presentationQueue == nullptr)
	{
		CCL_WARN ("Could not get a presentation queue from %s\n", deviceName.str ())
		deviceCandidates.remove (physicalDevice);
		terminate ();
		return false;
	}

	// Create command pool
	VkCommandPoolCreateInfo poolInfo {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	result = vkCreateCommandPool (logicalDevice, &poolInfo, nullptr, &commandPool);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s %s\n", "Failed to create a command pool.", string_VkResult (result))
		deviceCandidates.remove (physicalDevice);
		return false;
	}

	// Create descriptor set layout
	VkDescriptorSetLayoutBinding vertexShaderParamsLayoutBinding {};
	vertexShaderParamsLayoutBinding.binding = kTransformParameters;
	vertexShaderParamsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vertexShaderParamsLayoutBinding.descriptorCount = 1;
	vertexShaderParamsLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderParamsLayoutBinding.pImmutableSamplers = nullptr; // optional

	VkDescriptorSetLayoutBinding pixelShaderParamsLayoutBinding {};
	pixelShaderParamsLayoutBinding.binding = kMaterialParameters;
	pixelShaderParamsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pixelShaderParamsLayoutBinding.descriptorCount = 1;
	pixelShaderParamsLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pixelShaderParamsLayoutBinding.pImmutableSamplers = nullptr; // optional

	VkDescriptorSetLayoutBinding lightParamsLayoutBinding {};
	lightParamsLayoutBinding.binding = kLightParameters;
	lightParamsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightParamsLayoutBinding.descriptorCount = 1;
	lightParamsLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	lightParamsLayoutBinding.pImmutableSamplers = nullptr; // optional
	
	VkDescriptorSetLayoutBinding samplerLayoutBinding {};
	samplerLayoutBinding.binding = kLastShaderParameterIndex + 1;
	samplerLayoutBinding.descriptorCount = Vulkan3DDescriptorSet::kMaxTextureCount;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding bindings[] = { vertexShaderParamsLayoutBinding, pixelShaderParamsLayoutBinding, lightParamsLayoutBinding, samplerLayoutBinding };
	
	VkDescriptorSetLayoutCreateInfo layoutInfo {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = ARRAY_COUNT (bindings);
	layoutInfo.pBindings = bindings;

	result = vkCreateDescriptorSetLayout (logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s %s\n", "Failed to create a descriptor set layout.", string_VkResult (result))
		deviceCandidates.remove (physicalDevice);
		return false;
	}

	// Create pipeline cache
	
	VkPipelineCacheCreateInfo cacheCreateInfo {};
	cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    cacheCreateInfo.initialDataSize = 0;
    cacheCreateInfo.pInitialData = nullptr; // TODO consider storing/loading cache data to/from disk
    cacheCreateInfo.flags = 0;
    result = vkCreatePipelineCache (logicalDevice, &cacheCreateInfo, nullptr, &pipelineCache);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
	{
		CCL_WARN ("%s %s\n", "Failed to create a pipeline cache.", string_VkResult (result))
		deviceCandidates.remove (physicalDevice);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanClient::terminate ()
{
	initialized = false;

	if(pipelineCache != VK_NULL_HANDLE)
		vkDestroyPipelineCache (logicalDevice, pipelineCache, nullptr);

	if(descriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout (logicalDevice, descriptorSetLayout, nullptr);

	if(commandPool != VK_NULL_HANDLE)
		vkDestroyCommandPool (logicalDevice, commandPool, nullptr);
	
	Vulkan3DSupport::instance ().shutdown3D ();

	if(logicalDevice != nullptr)
		vkDestroyDevice (logicalDevice, nullptr);
	logicalDevice = nullptr;
	
	physicalDevice = nullptr;
	graphicsQueue = nullptr;
	presentationQueue = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanClient::isSupported ()
{
	static bool first = true;
	static bool result = false;
	
	if(first)
	{
		// try to initialize
		if(vulkanInstance == nullptr)
			createVulkanInstance ();
		result = vulkanInstance != nullptr;
		
		// initialize platform specifics
		if(result && !isInitialized ())
		{
			result = initializePlatform ();
		}
		
		if(result == false)
			CCL_WARN ("%s\n", "Vulkan is not supported!")

		first = false;
	}
	
	return result;
}
 
//////////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer VulkanClient::beginSingleTimeCommands ()
{
	VkCommandBufferAllocateInfo allocInfo {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = nullptr;
	VkResult result = vkAllocateCommandBuffers (logicalDevice, &allocInfo, &commandBuffer);
	ASSERT (result == VK_SUCCESS)
	if(result != VK_SUCCESS)
		return nullptr;

	VkCommandBufferBeginInfo beginInfo {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer (commandBuffer, &beginInfo);

	return commandBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanClient::endSingleTimeCommands (VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer (commandBuffer);

	VkSubmitInfo submitInfo {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit (graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle (graphicsQueue);

	vkFreeCommandBuffers (logicalDevice, commandPool, 1, &commandBuffer);
}

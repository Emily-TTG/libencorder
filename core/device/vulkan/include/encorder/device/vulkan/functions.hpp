#pragma once

#include <encorder/device/vulkan/common.hpp>

namespace encorder::vulkan {
	struct loader_functions {
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
		PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
		PFN_vkCreateInstance vkCreateInstance;
	};

	struct instance_functions {
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
		PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

		PFN_vkDestroyInstance vkDestroyInstance;
		PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
		PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;

		PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;
		PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2;
		PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2;
		PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2;

		PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR vkGetPhysicalDeviceVideoCapabilitiesKHR;
		PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR vkGetPhysicalDeviceVideoFormatPropertiesKHR;
		PFN_vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR;
	};

	struct device_functions {
		PFN_vkGetDeviceQueue vkGetDeviceQueue;
		PFN_vkDeviceWaitIdle vkDeviceWaitIdle;

		PFN_vkAllocateMemory vkAllocateMemory;
		PFN_vkFreeMemory vkFreeMemory;
		PFN_vkMapMemory vkMapMemory;
		PFN_vkUnmapMemory vkUnmapMemory;
		PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
		PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;

		PFN_vkCreateImage vkCreateImage;
		PFN_vkDestroyImage vkDestroyImage;
		PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2;
		PFN_vkBindImageMemory2 vkBindImageMemory2;
		PFN_vkCreateImageView vkCreateImageView;
		PFN_vkDestroyImageView vkDestroyImageView;

		PFN_vkCreateBuffer vkCreateBuffer;
		PFN_vkDestroyBuffer vkDestroyBuffer;
		PFN_vkGetBufferMemoryRequirements2 vkGetBufferMemoryRequirements2;
		PFN_vkBindBufferMemory2 vkBindBufferMemory2;

		PFN_vkCreateCommandPool vkCreateCommandPool;
		PFN_vkDestroyCommandPool vkDestroyCommandPool;
		PFN_vkResetCommandPool vkResetCommandPool;
		PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
		PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
		PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
		PFN_vkEndCommandBuffer vkEndCommandBuffer;
		PFN_vkResetCommandBuffer vkResetCommandBuffer;

		PFN_vkCmdPipelineBarrier2 vkCmdPipelineBarrier2;
		PFN_vkCmdCopyImage2 vkCmdCopyImage2;
		PFN_vkQueueSubmit2 vkQueueSubmit2;

		PFN_vkCreateSemaphore vkCreateSemaphore;
		PFN_vkDestroySemaphore vkDestroySemaphore;
		PFN_vkWaitSemaphores vkWaitSemaphores;
		PFN_vkSignalSemaphore vkSignalSemaphore;
		PFN_vkGetSemaphoreCounterValue vkGetSemaphoreCounterValue;

		PFN_vkCreateQueryPool vkCreateQueryPool;
		PFN_vkDestroyQueryPool vkDestroyQueryPool;
		PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
		PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
		PFN_vkCmdBeginQuery vkCmdBeginQuery;
		PFN_vkCmdEndQuery vkCmdEndQuery;

		PFN_vkCreateVideoSessionKHR vkCreateVideoSessionKHR;
		PFN_vkDestroyVideoSessionKHR vkDestroyVideoSessionKHR;
		PFN_vkGetVideoSessionMemoryRequirementsKHR vkGetVideoSessionMemoryRequirementsKHR;
		PFN_vkBindVideoSessionMemoryKHR vkBindVideoSessionMemoryKHR;

		PFN_vkCreateVideoSessionParametersKHR vkCreateVideoSessionParametersKHR;
		PFN_vkUpdateVideoSessionParametersKHR vkUpdateVideoSessionParametersKHR;
		PFN_vkDestroyVideoSessionParametersKHR vkDestroyVideoSessionParametersKHR;
		PFN_vkGetEncodedVideoSessionParametersKHR vkGetEncodedVideoSessionParametersKHR;

		PFN_vkCmdBeginVideoCodingKHR vkCmdBeginVideoCodingKHR;
		PFN_vkCmdEndVideoCodingKHR vkCmdEndVideoCodingKHR;
		PFN_vkCmdControlVideoCodingKHR vkCmdControlVideoCodingKHR;
		PFN_vkCmdEncodeVideoKHR vkCmdEncodeVideoKHR;
	};

	[[nodiscard]]
	loader_functions load_loader_functions(PFN_vkGetInstanceProcAddr) noexcept;

	[[nodiscard]]
	instance_functions load_instance_functions(PFN_vkGetInstanceProcAddr, VkInstance) noexcept;

	[[nodiscard]]
	device_functions load_device_functions(PFN_vkGetDeviceProcAddr, VkDevice) noexcept;

	// Returns the first entry point that failed to resolve.
	[[nodiscard]]
	const char* missing_instance_function(const instance_functions&) noexcept;

	[[nodiscard]]
	const char* missing_device_function(const device_functions&) noexcept;
}

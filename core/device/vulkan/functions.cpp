#include <encorder/device/vulkan/functions.hpp>

namespace encorder::vulkan {
	namespace {
#define ENC_INSTANCE_FUNCTIONS(entry) \
		entry(vkGetDeviceProcAddr) \
		entry(vkDestroyInstance) \
		entry(vkEnumeratePhysicalDevices) \
		entry(vkEnumerateDeviceExtensionProperties) \
		entry(vkGetPhysicalDeviceProperties2) \
		entry(vkGetPhysicalDeviceMemoryProperties2) \
		entry(vkGetPhysicalDeviceQueueFamilyProperties2) \
		entry(vkGetPhysicalDeviceFormatProperties2) \
		entry(vkGetPhysicalDeviceVideoCapabilitiesKHR) \
		entry(vkGetPhysicalDeviceVideoFormatPropertiesKHR) \
		entry(vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR)

		// TODO(Emily): Can we optionally defer to an existing VMA instance for the alloc stuff?
#define ENC_DEVICE_FUNCTIONS(entry) \
		entry(vkGetDeviceQueue) \
		entry(vkDeviceWaitIdle) \
		entry(vkAllocateMemory) \
		entry(vkFreeMemory) \
		entry(vkMapMemory) \
		entry(vkUnmapMemory) \
		entry(vkFlushMappedMemoryRanges) \
		entry(vkInvalidateMappedMemoryRanges) \
		entry(vkCreateImage) \
		entry(vkDestroyImage) \
		entry(vkGetImageMemoryRequirements2) \
		entry(vkBindImageMemory2) \
		entry(vkCreateImageView) \
		entry(vkDestroyImageView) \
		entry(vkCreateBuffer) \
		entry(vkDestroyBuffer) \
		entry(vkGetBufferMemoryRequirements2) \
		entry(vkBindBufferMemory2) \
		entry(vkCreateCommandPool) \
		entry(vkDestroyCommandPool) \
		entry(vkResetCommandPool) \
		entry(vkAllocateCommandBuffers) \
		entry(vkFreeCommandBuffers) \
		entry(vkBeginCommandBuffer) \
		entry(vkEndCommandBuffer) \
		entry(vkResetCommandBuffer) \
		entry(vkCmdPipelineBarrier2) \
		entry(vkCmdCopyImage2) \
		entry(vkQueueSubmit2) \
		entry(vkCreateSemaphore) \
		entry(vkDestroySemaphore) \
		entry(vkWaitSemaphores) \
		entry(vkSignalSemaphore) \
		entry(vkGetSemaphoreCounterValue) \
		entry(vkCreateQueryPool) \
		entry(vkDestroyQueryPool) \
		entry(vkGetQueryPoolResults) \
		entry(vkCmdResetQueryPool) \
		entry(vkCmdBeginQuery) \
		entry(vkCmdEndQuery) \
		entry(vkCreateVideoSessionKHR) \
		entry(vkDestroyVideoSessionKHR) \
		entry(vkGetVideoSessionMemoryRequirementsKHR) \
		entry(vkBindVideoSessionMemoryKHR) \
		entry(vkCreateVideoSessionParametersKHR) \
		entry(vkUpdateVideoSessionParametersKHR) \
		entry(vkDestroyVideoSessionParametersKHR) \
		entry(vkGetEncodedVideoSessionParametersKHR) \
		entry(vkCmdBeginVideoCodingKHR) \
		entry(vkCmdEndVideoCodingKHR) \
		entry(vkCmdControlVideoCodingKHR) \
		entry(vkCmdEncodeVideoKHR)
	}

	loader_functions load_loader_functions(const PFN_vkGetInstanceProcAddr get_proc) noexcept {
		loader_functions table{};

		if(!get_proc) return table;

		table.vkGetInstanceProcAddr = get_proc;

		// Null instance is the documented way to get these.
		table.vkEnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
				get_proc(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));

		table.vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
				get_proc(VK_NULL_HANDLE, "vkCreateInstance"));

		return table;
	}

	instance_functions load_instance_functions(
			const PFN_vkGetInstanceProcAddr get_proc,
			const VkInstance instance) noexcept {

		instance_functions table{};

		if(!get_proc) return table;

		table.vkGetInstanceProcAddr = get_proc;

#define ENC_LOAD(name) table.name = reinterpret_cast<PFN_##name>(get_proc(instance, #name));
		ENC_INSTANCE_FUNCTIONS(ENC_LOAD)
#undef ENC_LOAD

		return table;
	}

	device_functions load_device_functions(
			const PFN_vkGetDeviceProcAddr get_proc,
			const VkDevice device) noexcept {

		device_functions table{};

		if(!get_proc) return table;

#define ENC_LOAD(name) table.name = reinterpret_cast<PFN_##name>(get_proc(device, #name));
		ENC_DEVICE_FUNCTIONS(ENC_LOAD)
#undef ENC_LOAD

		return table;
	}

	const char* missing_instance_function(const instance_functions& table) noexcept {
		if(!table.vkGetInstanceProcAddr) return "vkGetInstanceProcAddr";

#define ENC_CHECK(name) if(!table.name) return #name;
		ENC_INSTANCE_FUNCTIONS(ENC_CHECK)
#undef ENC_CHECK

		return nullptr;
	}

	const char* missing_device_function(const device_functions& table) noexcept {
#define ENC_CHECK(name) if(!table.name) return #name;
		ENC_DEVICE_FUNCTIONS(ENC_CHECK)
#undef ENC_CHECK

		return nullptr;
	}
}

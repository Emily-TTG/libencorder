#ifndef ENC_ENCORDER_VULKAN_H
#define ENC_ENCORDER_VULKAN_H

#include <encorder/encorder.h>

// Includes must provide their own source for Vulkan types.
#if !defined(VK_VERSION_1_0)
# error "include <vulkan/vulkan.h> or <volk.h> before <encorder/encorder_vulkan.h>"
#endif

#define ENC_VULKAN_QUEUE_NONE (UINT32_MAX)

struct enc_vulkan_queue {
	uint32_t family;
	uint32_t index;
};

// The device must outlive the `enc_device` created from it.
struct enc_vulkan_device_info {
	ENC_PUBLIC_STRUCT

	PFN_vkGetInstanceProcAddr get_instance_proc_addr;

	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;

	// The `VkApplicationInfo::apiVersion` you created the instance with.
	uint32_t api_version;

	// Must name a family with `VK_QUEUE_VIDEO_ENCODE_BIT_KHR`.
	struct enc_vulkan_queue encode_queue;

	// `ENC_VULKAN_QUEUE_NONE` if you can't spare one.
	struct enc_vulkan_queue compute_queue;
	struct enc_vulkan_queue transfer_queue;

	// Exactly what you passed to `vkCreateDevice`.
	uint32_t enabled_extension_count;
	const char* const* enabled_extensions;

	// The chain you passed to `vkCreateDevice`; NULL skips feature checks.
	const VkPhysicalDeviceFeatures2* enabled_features;

	/*
	 * Supply both callbacks if you want encorder to submit for you, otherwise
	 * record-only.
	 */
	void (*queue_lock)(void*);
	void (*queue_unlock)(void*);
	void* queue_user;

	const VkAllocationCallbacks* allocator;
};

ENC_API void enc_vulkan_device_info_new(struct enc_vulkan_device_info*);

ENC_API enum enc_result enc_device_new_from_vulkan(
		struct enc_instance*,
		const struct enc_vulkan_device_info*,
		struct enc_device**);

#endif

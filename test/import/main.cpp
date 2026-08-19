#ifndef VK_NO_PROTOTYPES
# define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
#else
# include <dlfcn.h>
#endif

#include <encorder/encorder.h>
#include <encorder/encorder_vulkan.h>

#include <encorder/test/common.h>

#include <optional>
#include <vector>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
	// TODO(Emily): Kind of awful jankery atm, also means client-library mismatch of loaded library could occur
	//              depending on their test order.

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
	PFN_vkCreateInstance vkCreateInstance = nullptr;
	PFN_vkDestroyInstance vkDestroyInstance = nullptr;
	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = nullptr;
	PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = nullptr;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
	PFN_vkCreateDevice vkCreateDevice = nullptr;
	PFN_vkDestroyDevice vkDestroyDevice = nullptr;

	bool load_loader() {
#ifdef _WIN32
		const auto handle = LoadLibraryA("vulkan-1.dll");
		if(!handle) return false;

		const auto resolve = [handle](const char* const name) {
			return reinterpret_cast<void*>(GetProcAddress(handle, name));
		};
#else
		void* handle = dlopen("libvulkan.so.1", RTLD_LAZY | RTLD_LOCAL);
		if(!handle) handle = dlopen("libvulkan.so", RTLD_LAZY | RTLD_LOCAL);
		if(!handle) handle = dlopen("libvulkan.1.dylib", RTLD_LAZY | RTLD_LOCAL);
		if(!handle) return false;

		const auto resolve = [handle](const char* const name) {
			return dlsym(handle, name);
		};
#endif

		vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(resolve("vkGetInstanceProcAddr"));
		if(!vkGetInstanceProcAddr) return false;

		vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
		return vkCreateInstance != nullptr;
	}

	void load_instance_symbols(const VkInstance instance) {
#define LOAD(name) name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));
		LOAD(vkDestroyInstance)
		LOAD(vkEnumeratePhysicalDevices)
		LOAD(vkEnumerateDeviceExtensionProperties)
		LOAD(vkGetPhysicalDeviceQueueFamilyProperties)
		LOAD(vkCreateDevice)
		LOAD(vkDestroyDevice)
#undef LOAD
	}
}

namespace {
	struct host_renderer {
		VkInstance instance = VK_NULL_HANDLE;
		VkPhysicalDevice physical = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;

		std::uint32_t encode_family = ENC_VULKAN_QUEUE_NONE;
		std::uint32_t compute_family = ENC_VULKAN_QUEUE_NONE;

		std::vector<const char*> extensions;

		VkPhysicalDeviceVulkan13Features features13{};
		VkPhysicalDeviceFeatures2 features{};
	};

	bool has_extension(
			const std::vector<VkExtensionProperties>& available,
			const char* const name) {

		for(const auto& entry : available) {
			if(std::strcmp(entry.extensionName, name) == 0) return true;
		}

		return false;
	}

	std::optional<host_renderer> bring_up() {
		if(!load_loader()) {
			std::fprintf(stderr, "no vulkan loader\n");
			return std::nullopt;
		}

		host_renderer host;

		const VkApplicationInfo application{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pNext = nullptr,
				.pApplicationName = "encorder-import-test",
				.applicationVersion = 1,
				.pEngineName = "encorder-import-test",
				.engineVersion = 1,
				.apiVersion = VK_API_VERSION_1_3
		};

		const VkInstanceCreateInfo instance_create{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.pApplicationInfo = &application,
				.enabledLayerCount = 0,
				.ppEnabledLayerNames = nullptr,
				.enabledExtensionCount = 0,
				.ppEnabledExtensionNames = nullptr
		};

		if(vkCreateInstance(&instance_create, nullptr, &host.instance) != VK_SUCCESS) {
			std::fprintf(stderr, "vkCreateInstance failed\n");
			return std::nullopt;
		}

		load_instance_symbols(host.instance);

		std::uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(host.instance, &device_count, nullptr);

		std::vector<VkPhysicalDevice> handles(device_count);
		vkEnumeratePhysicalDevices(host.instance, &device_count, handles.data());

		for(const auto handle : handles) {
			std::uint32_t extension_count = 0;
			vkEnumerateDeviceExtensionProperties(handle, nullptr, &extension_count, nullptr);

			std::vector<VkExtensionProperties> available(extension_count);
			vkEnumerateDeviceExtensionProperties(
					handle, nullptr, &extension_count, available.data());

			if(!has_extension(available, VK_KHR_VIDEO_QUEUE_EXTENSION_NAME)) continue;
			if(!has_extension(available, VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME)) continue;

			std::uint32_t family_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(handle, &family_count, nullptr);

			std::vector<VkQueueFamilyProperties> families(family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(handle, &family_count, families.data());

			// TODO(Emily): Make some encorder_vulkan helpers similar to GLFW's extension applicators
			//              for clients to avoid a lot of this dance (i.e. keep as PnP as possible).

			for(std::uint32_t i = 0; i < family_count; ++i) {
				const auto flags = families[i].queueFlags;

				if((flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) &&
						host.encode_family == ENC_VULKAN_QUEUE_NONE) {

					host.encode_family = i;
				}

				if((flags & VK_QUEUE_COMPUTE_BIT) &&
						host.compute_family == ENC_VULKAN_QUEUE_NONE) {

					host.compute_family = i;
				}
			}

			if(host.encode_family == ENC_VULKAN_QUEUE_NONE) continue;

			host.physical = handle;

			host.extensions.push_back(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
			host.extensions.push_back(VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME);

			if(has_extension(available, VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME)) {
				host.extensions.push_back(VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME);
			}

			if(has_extension(available, VK_KHR_VIDEO_ENCODE_H265_EXTENSION_NAME)) {
				host.extensions.push_back(VK_KHR_VIDEO_ENCODE_H265_EXTENSION_NAME);
			}

			break;
		}

		if(host.physical == VK_NULL_HANDLE) {
			std::fprintf(stderr, "no vulkan device with an encode queue\n");
			return std::nullopt;
		}

		const float priority = 1.0f;

		const VkDeviceQueueCreateInfo queue_create{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueFamilyIndex = host.encode_family,
				.queueCount = 1,
				.pQueuePriorities = &priority
		};

		host.features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		host.features13.synchronization2 = VK_TRUE;

		host.features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		host.features.pNext = &host.features13;

		const VkDeviceCreateInfo device_create{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = &host.features,
				.flags = 0,
				.queueCreateInfoCount = 1,
				.pQueueCreateInfos = &queue_create,
				.enabledLayerCount = 0,
				.ppEnabledLayerNames = nullptr,
				.enabledExtensionCount = static_cast<std::uint32_t>(host.extensions.size()),
				.ppEnabledExtensionNames = host.extensions.data(),
				.pEnabledFeatures = nullptr
		};

		if(vkCreateDevice(host.physical, &device_create, nullptr, &host.device) != VK_SUCCESS) {
			std::fprintf(stderr, "vkCreateDevice failed\n");
			return std::nullopt;
		}

		return host;
	}

	void describe(host_renderer& host, enc_vulkan_device_info& info) {
		host.features.pNext = &host.features13;

		enc_vulkan_device_info_new(&info);

		info.get_instance_proc_addr = vkGetInstanceProcAddr;
		info.instance = host.instance;
		info.physical_device = host.physical;
		info.device = host.device;
		info.api_version = VK_API_VERSION_1_3;

		info.encode_queue.family = host.encode_family;
		info.encode_queue.index = 0;
		info.compute_queue.family = host.compute_family;

		info.enabled_extension_count = static_cast<std::uint32_t>(host.extensions.size());
		info.enabled_extensions = host.extensions.data();

		info.enabled_features = &host.features;
	}

	int failures = 0;

	// TODO(Emily): Janky temporary inline stuff, pull out to `common.h`.
	void expect(const bool condition, const char* const what) {
		std::printf("%s: %s\n", what, condition ? "OK" : "FAIL");

		if(!condition) ++failures;
	}
}

int main() {
	auto host = bring_up();

	if(!host) {
		std::fprintf(stderr, "skipping: no suitable vulkan device\n");
		return EXIT_SUCCESS;
	}

	enc_instance_info instance_info{};
	enc_instance_info_new(&instance_info);

	instance_info.application_name = "encorder-import";
	instance_info.log = &enc_test_on_log;
	instance_info.log_level = ENC_LOG_WARN;

	enc_instance* instance = nullptr;
	ENC_TEST_FATAL(enc_instance_new(&instance_info, &instance));

	enc_vulkan_device_info info{};
	describe(*host, info);

	std::printf("adoption:\n");

	enc_device* device = nullptr;
	ENC_TEST_FATAL(enc_device_new_from_vulkan(instance, &info, &device));

	expect(device != nullptr, "device adopted");
	expect(enc_device_can_encode(device), "adopted device reports encode capable");

	enc_device_info described{};
	expect(
			enc_device_get_info(device, &described) == ENC_RESULT_SUCCESS,
			"adopted device reports its info");

	enc_capabilities capabilities{};
	expect(
			enc_device_query_capabilities(device, ENC_BACKEND_VULKAN, ENC_CODEC_H264, &capabilities) == ENC_RESULT_SUCCESS,
			"adopted device answers capabilities");

	expect(
			capabilities.max_width > 0 && capabilities.max_height > 0,
			"capabilities carry a usable resolution range");

	std::printf(
			"\t%s: %ux%u, preferred input %s\n",
			described.name,
			capabilities.max_width,
			capabilities.max_height,
			enc_format_name(capabilities.preferred_input_format));

	enc_device_delete(device);
	enc_instance_delete(instance);

	vkDestroyDevice(host->device, nullptr);
	vkDestroyInstance(host->instance, nullptr);

	std::printf("%s\n", failures ? "FAIL" : "OK");

	return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}

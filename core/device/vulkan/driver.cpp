#include <encorder/device/vulkan/driver.hpp>
#include <encorder/device/vulkan/device.hpp>

#include <encorder/core/error.inl>
#include <encorder/core/library.inl>
#include <encorder/core/logger.inl>

namespace encorder::vulkan {
	namespace {
		constexpr std::array library_candidates{
#if defined(_WIN32)
				"vulkan-1.dll"
#elif defined(__APPLE__)
				"libvulkan.1.dylib",
				"libvulkan.dylib",
				"libMoltenVK.dylib"
#else
				"libvulkan.so.1",
				"libvulkan.so"
#endif
		};

		[[nodiscard]]
		bool has_extension(
				const std::vector<std::string>& extensions,
				const std::string_view name) noexcept {

			return std::ranges::contains(extensions, name);
		}
	}

	driver::driver(const logger& log) noexcept :
			log(log),
			loader{},
			functions{},
			instance(VK_NULL_HANDLE) {}

	driver::~driver() {
		if(instance != VK_NULL_HANDLE && functions.vkDestroyInstance) {
			functions.vkDestroyInstance(instance, nullptr);
		}
	}

	result<void> driver::initialise() {
		if(const auto loaded = load_library(); !loaded) return loaded;
		if(const auto created = create_instance(); !created) return created;

		return enumerate();
	}

	enc_native_kind driver::native_kind() const noexcept {
		return ENC_NATIVE_VULKAN;
	}

	const char* driver::name() const noexcept {
		return "vulkan";
	}

	std::span<const enc_device_info> driver::devices() const noexcept {
		return device_infos;
	}

	result<void> driver::load_library() {
		if(const auto opened = library.open(library_candidates); !opened) {
			return unexpect(
					ENC_RESULT_ERROR_INITIALIZATION_FAILED,
					"failed to open vulkan loader: {}",
					opened.error().get_context());
		}

		const auto get_symbol = library.lookup<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

		if(!get_symbol) {
			return unexpect(
					ENC_RESULT_ERROR_INITIALIZATION_FAILED,
					"could not find `vkGetInstanceProcAddr`");
		}

		loader = load_loader_functions(get_symbol);

		if(!loader.vkCreateInstance) {
			return unexpect(
					ENC_RESULT_ERROR_INITIALIZATION_FAILED,
					"could not find `vkCreateInstance`");
		}

		return {};
	}

	result<void> driver::create_instance() {
		std::uint32_t loader_version = VK_API_VERSION_1_0;

		if(loader.vkEnumerateInstanceVersion) {
			loader.vkEnumerateInstanceVersion(&loader_version);
		}

		/*
		 * TODO(Emily): We should be able to proceed with 1.2, but we do need
		 *              some extension aliasing
		 *              (`vkCmdPipelineBarrier2` -> `vkCmdPipelineBarrier2KHR`).
		 */
		if(loader_version < VK_API_VERSION_1_3) {
			return unexpect(
					ENC_RESULT_ERROR_UNSUPPORTED,
					"vulkan `{}.{}` detected; libencorder requires vulkan `>=1.3`",
					VK_API_VERSION_MAJOR(loader_version),
					VK_API_VERSION_MINOR(loader_version));
		}

		ENC_VULKAN_STRUCT(constexpr VkApplicationInfo application{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "libencorder",
				.applicationVersion = ENC_VERSION,
				.pEngineName = "libencorder",
				.engineVersion = ENC_VERSION,
				.apiVersion = VK_API_VERSION_1_3
		};)

		ENC_VULKAN_STRUCT(const VkInstanceCreateInfo create{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pApplicationInfo = &application
		};)

		if(const auto status = loader.vkCreateInstance(&create, nullptr, &instance);
				status != VK_SUCCESS) {
			return unexpect(
					ENC_RESULT_ERROR_INITIALIZATION_FAILED,
					"`vkCreateInstance` failed with `{}`",
					magic_enum::enum_name<VkResult>(status));
		}

		functions = load_instance_functions(loader.vkGetInstanceProcAddr, instance);

		const auto complete =
				functions.vkDestroyInstance
				&& functions.vkEnumeratePhysicalDevices
				&& functions.vkGetPhysicalDeviceProperties2
				&& functions.vkGetPhysicalDeviceQueueFamilyProperties2
				&& functions.vkEnumerateDeviceExtensionProperties;

		if(!complete) {
			// TODO(Emily): Get a more descriptive breakdown.
			return unexpect(
					ENC_RESULT_ERROR_INITIALIZATION_FAILED,
					"could not load required vulkan functions");
		}

		return {};
	}

	result<void> driver::enumerate() {
		std::uint32_t count = 0;

		if(const auto status = ENC_CHECK_VULKAN(
				functions.vkEnumeratePhysicalDevices(instance, &count, nullptr),
				ENC_RESULT_ERROR_NO_DEVICE); !status) {

			return std::unexpected(status.error());
		}

		if(!count) return unexpect(ENC_RESULT_ERROR_NO_DEVICE, "no vulkan physical devices");

		std::vector<VkPhysicalDevice> handles(count);
		functions.vkEnumeratePhysicalDevices(instance, &count, handles.data());

		for(const auto handle : handles) {
			auto inspected = inspect(handle);

			log.log(
					ENC_LOG_DEBUG,
					"vulkan: device `{}`; encode codecs `{}`; encode family `{}`",
					inspected.info.name,
					magic_enum::enum_flags_name<enc_codec>(inspected.encode_codecs),
					inspected.encode_family
							? std::to_string(*inspected.encode_family)
							: "none");

			// TODO(Emily): In future, reserve these for falling back to pure-compute.
			if(!inspected.encode_codecs) continue;

			device_infos.push_back(inspected.info);
			physical_devices.push_back(std::move(inspected));
		}

		if(physical_devices.empty()) {
			return unexpect(
					ENC_RESULT_ERROR_NO_DEVICE,
					"no vulkan devices support video encode queue");
		}

		return {};
	}

	physical_device_info driver::inspect(const VkPhysicalDevice handle) const {
		physical_device_info result{};

		result.handle = handle;

		ENC_VULKAN_STRUCT(VkPhysicalDeviceIDProperties identity{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES
		};)

		ENC_VULKAN_STRUCT(VkPhysicalDeviceDriverProperties driver_properties{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
				.pNext = &identity
		};)

		ENC_VULKAN_STRUCT(VkPhysicalDeviceProperties2 properties{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
				.pNext = &driver_properties
		};)

		functions.vkGetPhysicalDeviceProperties2(handle, &properties);

		result.api_version = properties.properties.apiVersion;
		result.driver_id = driver_properties.driverID;

		result.info.struct_size = sizeof(enc_device_info);
		result.info.version = 0;
		result.info.native_kind = ENC_NATIVE_VULKAN;
		result.info.vendor_id = properties.properties.vendorID;
		result.info.device_id = properties.properties.deviceID;
		result.info.driver_version = properties.properties.driverVersion;
		result.info.is_hardware = properties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU;

		std::memcpy(result.info.uuid, identity.deviceUUID, sizeof(result.info.uuid));

		const std::string_view device_name = properties.properties.deviceName;
		const auto writable = std::min(device_name.size(), sizeof(result.info.name) - 1);

		std::memcpy(result.info.name, device_name.data(), writable);
		result.info.name[writable] = '\0';

		std::vector<VkExtensionProperties> extension_properties;

		// TODO(Emily): Just kind of pulled this number out of thin air for
		//              re-querying. Does Khronos not tell us how to retry here?
		for(std::uint32_t attempt = 0; attempt < 4; ++attempt) {
			std::uint32_t extension_count = 0;

			auto status = functions.vkEnumerateDeviceExtensionProperties(
					handle,
					nullptr,
					&extension_count,
					nullptr);

			if(status == VK_SUCCESS && extension_count) {
				extension_properties.resize(extension_count);

				status = functions.vkEnumerateDeviceExtensionProperties(
						handle,
						nullptr,
						&extension_count,
						extension_properties.data());
			}

			if(status == VK_INCOMPLETE) continue;

			if(status != VK_SUCCESS) {
				log.log(
						ENC_LOG_WARN,
						"vulkan: could not enumerate device extensions for `{}`: `{}`",
						result.info.name,
						magic_enum::enum_name<VkResult>(status));

				extension_properties.clear();
			}

			// A driver may report fewer than it sized for.
			extension_properties.resize(std::min<std::size_t>(extension_count, extension_properties.size()));

			break;
		}

		result.extensions.reserve(extension_properties.size());

		for(const auto& [ name, version ] : extension_properties) {
			result.extensions.emplace_back(name);
		}

		const auto video_queue = has_extension(result.extensions, VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
		const auto encode_queue = has_extension(result.extensions, VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME);

		std::uint32_t family_count = 0;
		functions.vkGetPhysicalDeviceQueueFamilyProperties2(handle, &family_count, nullptr);

		std::vector<VkQueueFamilyProperties2> family_properties(family_count);
		std::vector<VkQueueFamilyVideoPropertiesKHR> video_properties(family_count);

		for(std::uint32_t i = 0; i < family_count; ++i) {
			ENC_VULKAN_STRUCT(video_properties[i] = VkQueueFamilyVideoPropertiesKHR{
					.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR
			};)

			ENC_VULKAN_STRUCT(family_properties[i] = VkQueueFamilyProperties2{
					.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2,
					.pNext = video_queue ? &video_properties[i] : nullptr
			};)
		}

		functions.vkGetPhysicalDeviceQueueFamilyProperties2(
				handle,
				&family_count,
				family_properties.data());

		result.families.reserve(family_count);

		for(std::uint32_t i = 0; i < family_count; ++i) {
			const auto flags = family_properties[i].queueFamilyProperties.queueFlags;

			result.families.push_back({
					.index = i,
					.queue_count = family_properties[i].queueFamilyProperties.queueCount,
					.flags = flags,
					.codec_operations = video_queue ? video_properties[i].videoCodecOperations : 0u});

			// ReSharper disable once CppRedundantParentheses
			if(!result.compute_family && (flags & VK_QUEUE_COMPUTE_BIT)) {
				result.compute_family = i;
			}

			if(!encode_queue) continue;
			if(!(flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)) continue;

			if(!result.encode_family) result.encode_family = i;

			for(const auto& [ flag, codec, extension ] : codec_mappings) {
				if(!(video_properties[i].videoCodecOperations & flag)) {
					continue;
				}

				if(!has_extension(result.extensions, extension)) continue;

				result.encode_codecs = static_cast<enc_codec>(result.encode_codecs | codec);
			}
		}

		result.info.backends = result.encode_codecs ? ENC_BACKEND_VULKAN : ENC_BACKEND_NONE;
		result.info.codecs = result.encode_codecs;

		return result;
	}

	result<std::unique_ptr<device>> driver::open(const std::uint32_t index) {
		if(index >= physical_devices.size()) {
			return unexpect(
					ENC_RESULT_ERROR_NO_DEVICE,
					"vulkan device index `{}` out of range",
					index);
		}

		return std::make_unique<query_device>(log, functions, physical_devices[index]);
	}

	result<std::unique_ptr<encorder::driver>> make_driver(const logger& log) {
		auto created = std::make_unique<driver>(log);

		if(const auto ready = created->initialise(); !ready) {
			return std::unexpected(ready.error());
		}

		return created;
	}
}

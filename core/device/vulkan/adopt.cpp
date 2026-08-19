#include <encorder/device/vulkan/driver.hpp>
#include <encorder/device/vulkan/device.hpp>

#include <encorder/core/error.inl>
#include <encorder/core/logger.inl>

#include <encorder/encorder_vulkan.h>

using namespace magic_enum::bitwise_operators;

namespace encorder::vulkan {
	namespace {
		[[nodiscard]]
		bool enabled(const enc_vulkan_device_info& info, const std::string_view name) noexcept {
			const auto names = std::span(info.enabled_extensions, info.enabled_extension_count);

			return std::ranges::any_of(
					names,
					[name](const char* const entry) { return entry && name == entry; });
		}

		// `VkPhysicalDevice` aren't necessarily unique across devices. Using `deviceUUID`.
		[[nodiscard]]
		result<std::array<std::uint8_t, VK_UUID_SIZE>> read_uuid(
				const instance_functions& functions,
				const VkPhysicalDevice handle) {

			if(!functions.vkGetPhysicalDeviceProperties2) {
				return unexpect(
						ENC_RESULT_ERROR_INITIALIZATION_FAILED,
						"could not find `vkGetPhysicalDeviceProperties2`");
			}

			ENC_VULKAN_STRUCT(VkPhysicalDeviceIDProperties identity{
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES
			};)

			ENC_VULKAN_STRUCT(VkPhysicalDeviceProperties2 properties{
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
					.pNext = &identity
			};)

			functions.vkGetPhysicalDeviceProperties2(handle, &properties);

			std::array<std::uint8_t, VK_UUID_SIZE> uuid{};
			std::memcpy(uuid.data(), identity.deviceUUID, uuid.size());

			return uuid;
		}

		// `synchronization2` is core in 1.3 but still opt-in.
		[[nodiscard]]
		bool synchronisation2_enabled(const VkPhysicalDeviceFeatures2* chain) noexcept {
			for(const auto* next = static_cast<const VkBaseInStructure*>(
					static_cast<const void*>(chain)); next; next = next->pNext) {

				if(next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES) {
					const auto* const features =
							reinterpret_cast<const VkPhysicalDeviceVulkan13Features*>(next);

					if(features->synchronization2) return true;
				}

				if(next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES) {
					const auto* const features =
							reinterpret_cast<const VkPhysicalDeviceSynchronization2Features*>(next);

					if(features->synchronization2) return true;
				}
			}

			return false;
		}
	}

	result<std::unique_ptr<device>> driver::adopt(const void* const descriptor) {
		const auto read = ENC_READ_STRUCT(enc_vulkan_device_info, descriptor, 0);
		if(!read) return std::unexpected(read.error());

		const auto& info = *read;

		ENC_NULL_CHECK(info.get_instance_proc_addr);
		ENC_NULL_CHECK(info.instance);
		ENC_NULL_CHECK(info.physical_device);
		ENC_NULL_CHECK(info.device);

		if(info.api_version < VK_API_VERSION_1_3) {
			return unexpect(
					ENC_RESULT_ERROR_VERSION_MISMATCH,
					"libencorder requires vulkan `>=1.3`",
					VK_API_VERSION_MAJOR(info.api_version),
					VK_API_VERSION_MINOR(info.api_version));
		}

		if(static_cast<bool>(info.queue_lock) != static_cast<bool>(info.queue_unlock)) {
			return unexpect(
					ENC_RESULT_ERROR_INVALID_ARGUMENT,
					"`queue_lock` and `queue_unlock` must be supplied together");
		}

		/* Everything from here on dispatches through the caller's loader. */
		auto imported_api = load_instance_functions(info.get_instance_proc_addr, info.instance);

		if(const auto* const missing = missing_instance_function(imported_api)) {
			return unexpect(
					ENC_RESULT_ERROR_INITIALIZATION_FAILED,
					"could not find `{}`",
					missing);
		}

		const auto uuid = read_uuid(imported_api, info.physical_device);
		if(!uuid) return std::unexpected(uuid.error());

		const auto match = std::ranges::find_if(
				physical_devices,
				[&](const physical_device_info& entry) {
					return std::equal(uuid->begin(), uuid->end(), entry.info.uuid);
				});

		if(match == physical_devices.end()) {
			return unexpect(
					ENC_RESULT_ERROR_DEVICE_UUID_MISMATCH,
					"the supplied physical device is not encode capable");
		}

		if(!enabled(info, VK_KHR_VIDEO_QUEUE_EXTENSION_NAME) ||
				!enabled(info, VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME)) {

			return unexpect(
					ENC_RESULT_ERROR_IMPORT_MISMATCH,
					"`{}` and `{}` must both be enabled on the device",
					VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
					VK_KHR_VIDEO_ENCODE_QUEUE_EXTENSION_NAME);
		}

		// Only do enabled codecs, not all supported
		auto codecs = ENC_CODEC_NONE;

		for(const auto& mapping : codec_mappings) {
			if((match->encode_codecs & mapping.codec) && enabled(info, mapping.extension)) {
				codecs = codecs | mapping.codec;
			}
		}

		if(codecs == ENC_CODEC_NONE) {
			return unexpect(
					ENC_RESULT_ERROR_CODEC_UNSUPPORTED,
					"no codecs enabled; `{}` supports `{}`",
					match->info.name,
					magic_enum::enum_flags_name<enc_codec>(match->encode_codecs));
		}

		if(info.encode_queue.family == ENC_VULKAN_QUEUE_NONE ||
				info.encode_queue.family >= match->families.size()) {

			return unexpect(
					ENC_RESULT_ERROR_INVALID_ARGUMENT,
					"encode queue family `{}` is out of range; `{}` has {}",
					info.encode_queue.family,
					match->info.name,
					match->families.size());
		}

		const auto& family = match->families[info.encode_queue.family];

		if(!(family.flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)) {
			return unexpect(
					ENC_RESULT_ERROR_IMPORT_MISMATCH,
					"queue family `{}` cannot encode video; family `{}` can",
					info.encode_queue.family,
					match->encode_family ? std::to_string(*match->encode_family) : "none");
		}

		if(info.encode_queue.index >= family.queue_count) {
			return unexpect(
					ENC_RESULT_ERROR_INVALID_ARGUMENT,
					"encode queue index `{}` is out of range; family `{}` has {} queue{}",
					info.encode_queue.index,
					info.encode_queue.family,
					family.queue_count,
					family.queue_count != 1 ? "s" : "");
		}

		if(!info.enabled_features) {
			log.log(
					ENC_LOG_WARN,
					"vulkan: no feature chain supplied; assuming `synchronization2` is enabled");
		}
		else if(!synchronisation2_enabled(info.enabled_features)) {
			return unexpect(
					ENC_RESULT_ERROR_IMPORT_MISMATCH,
					"`synchronization2` must be enabled");
		}

		auto device_api = load_device_functions(imported_api.vkGetDeviceProcAddr, info.device);

		if(const auto* const missing = missing_device_function(device_api)) {
			return unexpect(
					ENC_RESULT_ERROR_INITIALIZATION_FAILED,
					"could not find `{}`",
					missing);
		}

		imported_context context{};

		context.instance = info.instance;
		context.physical_device = info.physical_device;
		context.device = info.device;
		context.api_version = info.api_version;
		context.encode_family = info.encode_queue.family;
		context.encode_index = info.encode_queue.index;
		context.queue_lock = info.queue_lock;
		context.queue_unlock = info.queue_unlock;
		context.queue_user = info.queue_user;
		context.allocator = info.allocator;

		if(info.compute_queue.family != ENC_VULKAN_QUEUE_NONE) {
			context.compute_family = info.compute_queue.family;
		}

		if(info.transfer_queue.family != ENC_VULKAN_QUEUE_NONE) {
			context.transfer_family = info.transfer_queue.family;
		}

		auto record = *match;

		record.handle = info.physical_device;
		record.encode_codecs = codecs;
		record.info.codecs = codecs;
		record.encode_family = info.encode_queue.family;

		if(context.compute_family) record.compute_family = context.compute_family;

		log.log(
				ENC_LOG_INFO,
				"vulkan: adopted `{}`; codecs `{}`; encode queue {}.{}; {}",
				record.info.name,
				magic_enum::enum_flags_name<enc_codec>(codecs),
				context.encode_family,
				context.encode_index,
				context.queue_lock ? "encorder may submit" : "record-only");

		return std::make_unique<adopted_device>(log, context, imported_api, device_api, record);
	}
}

ENC_API void enc_vulkan_device_info_new(enc_vulkan_device_info* const info) {
	if(!info) return;

	*info = enc_vulkan_device_info{};

	info->struct_size = sizeof(enc_vulkan_device_info);
	info->version = 0;

	info->encode_queue = { .family = ENC_VULKAN_QUEUE_NONE, .index = 0 };
	info->compute_queue = { .family = ENC_VULKAN_QUEUE_NONE, .index = 0 };
	info->transfer_queue = { .family = ENC_VULKAN_QUEUE_NONE, .index = 0 };
}

ENC_API enc_result enc_device_new_from_vulkan(
		enc_instance* const instance,
		const enc_vulkan_device_info* const info,
		enc_device** const out) {

	return enc_device_new_from_native(instance, ENC_NATIVE_VULKAN, info, out);
}

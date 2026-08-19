#include <encorder/core/instance.hpp>
#include <encorder/core/error.inl>
#include <encorder/core/logger.inl>

using namespace magic_enum::bitwise_operators;

namespace encorder {
	namespace {
		[[nodiscard]]
		result<std::unique_ptr<enc_instance>> build_instance(const enc_instance_info& info) {
			if(ENC_VERSION_MAJOR(info.api_version) != ENC_VERSION_MAJOR(ENC_VERSION)) {
				return unexpect(
						ENC_RESULT_ERROR_VERSION_MISMATCH,
						"caller built against `{}.{}.{}`, library is `{}.{}.{}`",
						ENC_VERSION_MAJOR(info.api_version),
						ENC_VERSION_MINOR(info.api_version),
						ENC_VERSION_PATCH(info.api_version),
						ENC_VERSION_MAJOR(ENC_VERSION),
						ENC_VERSION_MINOR(ENC_VERSION),
						ENC_VERSION_PATCH(ENC_VERSION));
			}

			auto instance = std::make_unique<enc_instance>();

			instance->logger = logger(info.log, info.log_user, info.log_level);
			instance->requested_backends = info.enabled_backends;
			instance->available_backends = ENC_BACKEND_NONE;

			for(const auto& registration : registered_drivers()) {
				auto driver = registration.create(instance->logger);
				if(!driver) {
					instance->logger.log(
							ENC_LOG_INFO,
							"{} driver unavailable: {} ({})",
							registration.name,
							driver.error().get_context(),
							enc_result_name(driver.error().get_code()));

					continue;
				}

				const auto index = static_cast<std::uint32_t>(instance->drivers.size());
				const auto count = (*driver)->devices().size();

				instance->logger.log(
						ENC_LOG_INFO,
						"`{}` driver ready, {} device{}",
						registration.name,
						count,
						count != 1 ? "s" : "");

				for(std::size_t local = 0; local < count; ++local) {
					instance->device_slots.push_back({
							.owner = index,
							.local = static_cast<std::uint32_t>(local)
					});
				}

				instance->drivers.push_back(std::move(*driver));
			}

			for(const auto& [ owner, local ] : instance->device_slots) {
				instance->available_backends =
						instance->available_backends |
						instance->drivers[owner]->devices()[local].backends;
			}

			if(instance->drivers.empty()) {
				return unexpect(
						ENC_RESULT_ERROR_NO_BACKEND,
						"no device driver could be initialised");
			}

			return instance;
		}
	}

}

encorder::result<encorder::device*> enc_instance::query_device(const std::uint32_t index) {
	using namespace encorder;

	if(index >= device_slots.size()) {
		return unexpect(
				ENC_RESULT_ERROR_NO_DEVICE,
				"device index {} out of range, {} available",
				index,
				device_slots.size());
	}

	const std::scoped_lock guard(query_mutex);

	query_devices.resize(device_slots.size());

	if(const auto& cached = query_devices[index]) return cached.get();

	const auto& [ owner, local ] = device_slots[index];

	auto opened = drivers[owner]->open(local);
	if(!opened) return std::unexpected(opened.error());

	query_devices[index] = std::move(*opened);

	return query_devices[index].get();
}

namespace encorder {

	ENC_API void enc_instance_info_new(enc_instance_info* const info) {
		if(!info) return;

		*info = enc_instance_info{};

		info->struct_size = sizeof(enc_instance_info);
		info->version = 0;
		info->api_version = ENC_VERSION;
		info->log_level = ENC_LOG_OFF;
	}

	ENC_API enc_result enc_instance_new(const enc_instance_info* const info, enc_instance** const out) {
		using namespace encorder;

		if(!out) return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);

		*out = nullptr;

		const auto checked = ENC_READ_STRUCT(enc_instance_info, info, 0);
		if(!checked) return set_error_result(checked.error());

		auto instance = build_instance(*checked);
		if(!instance) return set_error_result(instance.error());

		*out = instance->release();

		return set_error_result(ENC_RESULT_SUCCESS);
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	ENC_API void enc_instance_delete(enc_instance* const instance) {
		delete instance;
	}

	ENC_API enc_backend enc_instance_backends(const enc_instance* const instance) {
		return instance ? instance->available_backends : ENC_BACKEND_NONE;
	}

	ENC_API enc_result enc_enumerate_devices(
			// ReSharper disable once CppParameterMayBeConstPtrOrRef
			enc_instance* const instance,
			uint32_t* const count,
			enc_device_info* const out) {

		using namespace encorder;

		if(!instance || !count) return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);

		const auto available = static_cast<std::uint32_t>(instance->device_slots.size());

		if(!out) {
			*count = available;

			return set_error_result(ENC_RESULT_SUCCESS);
		}

		const auto writable = std::min(*count, available);

		for(std::uint32_t i = 0; i < writable; ++i) {
			const auto& [ owner, local ] = instance->device_slots[i];

			out[i] = instance->drivers[owner]->devices()[local];
		}

		*count = writable;

		return set_error_result(writable < available ? ENC_RESULT_INCOMPLETE : ENC_RESULT_SUCCESS);
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	ENC_API void enc_device_delete(enc_device* const device) {
		delete device;
	}

	ENC_API enc_result enc_device_get_info(
			const enc_device* const device,
			enc_device_info* const out) {

		using namespace encorder;

		if(!device || !out) return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);

		*out = device->implementation->info();

		return set_error_result(ENC_RESULT_SUCCESS);
	}
}

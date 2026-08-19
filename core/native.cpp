#include <encorder/core/instance.hpp>
#include <encorder/core/error.inl>
#include <encorder/core/logger.inl>

namespace encorder {
	ENC_API enc_result enc_device_new_from_native(
			enc_instance* const instance,
			const enc_native_kind kind,
			const void* const descriptor,
			enc_device** const out) {

		if(!instance || !descriptor || !out) {
			return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);
		}

		*out = nullptr;

		for(const auto& driver : instance->drivers) {
			if(driver->native_kind() != kind) continue;

			auto adopted = driver->adopt(descriptor);
			if(!adopted) return set_error_result(adopted.error());

			auto device = std::make_unique<enc_device>(enc_device{
					.owner = instance,
					.implementation = std::move(*adopted)
			});

			*out = device.release();

			return set_error_result(ENC_RESULT_SUCCESS);
		}

		return set_error_result(error(
				ENC_RESULT_ERROR_NO_BACKEND,
				std::format(
						"no driver provides `{}` devices",
						magic_enum::enum_name<enc_native_kind>(kind))));
	}

	ENC_API bool enc_device_can_encode(const enc_device* const device) {
		return device && device->implementation->encode_capable();
	}
}

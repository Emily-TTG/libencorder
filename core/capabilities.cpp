#include <encorder/core/instance.hpp>
#include <encorder/core/error.inl>

using namespace magic_enum::bitwise_operators;

namespace encorder {
	namespace {
		[[nodiscard]]
		result<void> check_codec(const enc_codec codec) {
			if(std::popcount(magic_enum::enum_underlying(codec)) == 1) return {};

			return unexpect(
					ENC_RESULT_ERROR_INVALID_ARGUMENT,
					"`codec` must name exactly one codec, got `{}`",
					magic_enum::enum_flags_name<enc_codec>(codec));
		}

		[[nodiscard]]
		result<void> check_backend(const device& implementation, const enc_backend backend) {
			if(backend == ENC_BACKEND_NONE) return {};

			if(std::popcount(static_cast<std::uint32_t>(backend)) != 1) {
				return unexpect(
						ENC_RESULT_ERROR_INVALID_ARGUMENT,
						"`backend` must name at most one backend, got `{}`",
						magic_enum::enum_flags_name<enc_backend>(backend));
			}

			if(!(backend & implementation.backends())) {
				return unexpect(
						ENC_RESULT_ERROR_BACKEND_MISMATCH,
						"device does not provide `{}`, only `{}`",
						enc_backend_name(backend),
						magic_enum::enum_flags_name<enc_backend>(implementation.backends()));
			}

			return {};
		}

		[[nodiscard]]
		result<device*> resolve(enc_instance* const instance, const std::uint32_t index) {
			if(!instance) return unexpect(ENC_RESULT_ERROR_INVALID_ARGUMENT);

			return instance->query_device(index);
		}
	}

	ENC_API void enc_config_new(enc_config* const config, const enc_codec codec) {
		if(!config) return;

		*config = enc_config{};

		config->struct_size = sizeof(enc_config);
		config->version = 0;
		config->codec = codec;

		config->frame_rate_numerator = 60;
		config->frame_rate_denominator = 1;

		config->input_format = ENC_FORMAT_NV12;

		config->color.struct_size = sizeof(enc_color_parameters);
		config->color.matrix = ENC_SIGNAL_BT709_6;
		config->color.primaries = ENC_SIGNAL_BT709_6;
		config->color.transfer = ENC_SIGNAL_BT709_6;
		config->color.range = ENC_RANGE_LIMITED;
		config->color.siting = ENC_CHROMA_SITING_LEFT;
		config->color.dither = true;

		config->rate.struct_size = sizeof(enc_rate_control);
		config->rate.mode = ENC_RATE_AVERAGE_BITRATE;
		config->rate.parameters.average_bitrate.bitrate = 20u * 1000u * 1000u;
		config->rate.parameters.average_bitrate.max_bitrate = 30u * 1000u * 1000u;

		config->gop.struct_size = sizeof(enc_gop);
		config->gop.keyframe_interval = 120;
		config->gop.reference_frames = 1;
		config->gop.temporal_layers = 1;
		config->gop.closed = true;

		config->accel = ENC_ACCELERATION_REQUIRE_HARDWARE;
		config->overflow = ENC_OVERFLOW_FAIL;
		config->preferred_backend = ENC_BACKEND_NONE;
		config->conversion = ENC_CONVERSION_FORBID;
		config->async_depth = 2;

		/* Delta-only codecs cannot express a keyframe interval. */
		if(!enc_codec_has_keyframes(codec)) {
			config->gop.keyframe_interval = 0;
			config->gop.reference_frames = 0;

			config->rate.mode = ENC_RATE_CODEC_TIER;
			config->rate.parameters = {};
		}
	}

	ENC_API enc_result enc_instance_query_capabilities(
			enc_instance* const instance,
			const uint32_t index,
			const enc_backend backend,
			const enc_codec codec,
			enc_capabilities* const out) {

		if(!out) return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);

		const auto resolved = resolve(instance, index);
		if(!resolved) return set_error_result(resolved.error());

		if(const auto checked = check_codec(codec); !checked) {
			return set_error_result(checked.error());
		}

		if(const auto checked = check_backend(**resolved, backend); !checked) {
			return set_error_result(checked.error());
		}

		auto queried = (*resolved)->query_capabilities(backend, codec);
		if(!queried) return set_error_result(queried.error());

		*out = *queried;

		return set_error_result(ENC_RESULT_SUCCESS);
	}

	ENC_API enc_result enc_instance_query_concurrency(
			enc_instance* const instance,
			const uint32_t index,
			const enc_codec codec,
			enc_concurrency_capabilities* const out) {

		if(!out) return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);

		const auto resolved = resolve(instance, index);
		if(!resolved) return set_error_result(resolved.error());

		auto queried = (*resolved)->query_concurrency(codec);
		if(!queried) return set_error_result(queried.error());

		queried->active_sessions = instance->sessions.active(codec);

		*out = *queried;

		return set_error_result(ENC_RESULT_SUCCESS);
	}

	ENC_API enc_result enc_instance_query_format(
			enc_instance* const instance,
			const uint32_t index,
			const enc_backend backend,
			const enc_codec codec,
			const enc_format format,
			enc_surface_tier* const out) {

		if(!out) return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);

		*out = ENC_SURFACE_TIER_NONE;

		const auto resolved = resolve(instance, index);
		if(!resolved) return set_error_result(resolved.error());

		if(const auto checked = check_codec(codec); !checked) {
			return set_error_result(checked.error());
		}

		if(const auto checked = check_backend(**resolved, backend); !checked) {
			return set_error_result(checked.error());
		}

		auto queried = (*resolved)->query_format(backend, codec, format);
		if(!queried) return set_error_result(queried.error());

		*out = *queried;

		return set_error_result(ENC_RESULT_SUCCESS);
	}

	ENC_API enc_result enc_device_query_capabilities(
			// ReSharper disable once CppParameterMayBeConstPtrOrRef
			enc_device* const device,
			const enc_backend backend,
			const enc_codec codec,
			enc_capabilities* const out) {

		if(!device || !out) return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);

		if(const auto checked = check_codec(codec); !checked) {
			return set_error_result(checked.error());
		}

		if(const auto checked = check_backend(*device->implementation, backend); !checked) {
			return set_error_result(checked.error());
		}

		auto queried = device->implementation->query_capabilities(backend, codec);
		if(!queried) return set_error_result(queried.error());

		*out = *queried;

		return set_error_result(ENC_RESULT_SUCCESS);
	}

	ENC_API enc_result enc_device_query_concurrency(
			// ReSharper disable once CppParameterMayBeConstPtrOrRef
			enc_device* const device,
			const enc_codec codec,
			enc_concurrency_capabilities* const out) {

		if(!device || !out) return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);

		auto queried = device->implementation->query_concurrency(codec);
		if(!queried) return set_error_result(queried.error());

		if(device->owner) queried->active_sessions = device->owner->sessions.active(codec);

		*out = *queried;

		return set_error_result(ENC_RESULT_SUCCESS);
	}
}

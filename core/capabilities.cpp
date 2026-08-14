#include <encorder/core/instance.hpp>

namespace encorder {
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
		config->async_depth = 2;

		/* Delta-only codecs cannot express a keyframe interval. */
		if(!enc_codec_has_keyframes(codec)) {
			config->gop.keyframe_interval = 0;
			config->gop.reference_frames = 0;

			config->rate.mode = ENC_RATE_CODEC_TIER;
			config->rate.parameters = {};
		}
	}

	ENC_API enc_result enc_device_query_capabilities(
			// ReSharper disable once CppParameterMayBeConstPtrOrRef
			enc_device* const device,
			const enc_backend backend,
			const enc_codec codec,
			// ReSharper disable once CppParameterMayBeConstPtrOrRef
			enc_capabilities* const out) {

		if(!device || !out) return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);

		/* TODO(Emily): Vulkan encode capability queries. */
		return set_error_result(error(
				ENC_RESULT_ERROR_UNSUPPORTED,
				std::format(
						"capabilities for `{}/{}` not implemented yet",
						enc_backend_name(backend),
						enc_codec_name(codec))));
	}

	ENC_API enc_result enc_device_query_concurrency(
			// ReSharper disable once CppParameterMayBeConstPtrOrRef
			enc_device* const device,
			const enc_codec,
			enc_concurrency_capabilities* const out) {

		if(!device || !out) return set_error_result(ENC_RESULT_ERROR_INVALID_ARGUMENT);

		*out = enc_concurrency_capabilities{};

		out->struct_size = sizeof(enc_concurrency_capabilities);
		out->version = 0;

		/* No Vulkan query exists for this. */
		out->max_sessions = 0;
		out->source = ENC_SESSION_LIMIT_UNKNOWN;
		out->active_sessions = 0;
		out->encode_engine_count = 0;
		out->sessions_share_engine = false;

		return set_error_result(ENC_RESULT_SUCCESS);
	}
}

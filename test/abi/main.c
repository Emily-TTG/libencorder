#include <encorder/encorder.h>

#include <encorder/test/common.h>

int main(void) {
	struct enc_instance_info info;
	struct enc_config config;
	struct enc_instance* instance = 0;
	struct enc_device_info devices[4];
	uint32_t count = sizeof(devices) / sizeof(devices[0]);
	uint32_t index;
	enum enc_format preferred = ENC_FORMAT_UNDEFINED;

	enc_instance_info_new(&info);
	enc_config_new(&config, ENC_CODEC_H264);

	config.rate.mode = ENC_RATE_CONSTANT_QUANTISER;
	config.rate.parameters.constant_quantizer.quantizer_iframes = 20;

	config.rate.mode = ENC_RATE_CONSTANT_QUALITY;
	config.rate.parameters.constant_quality.quality = 24;
	config.rate.parameters.constant_quality.max_bitrate = ENC_BITRATE_UNCAPPED;
	config.rate.parameters.constant_quality.buffer.size_bits = ENC_BUFFER_UNCONSTRAINED;

	config.rate.mode = ENC_RATE_CONSTANT_BITRATE;
	config.rate.parameters.constant_bitrate.pad_to_rate = true;

	config.rate.mode = ENC_RATE_CODEC_TIER;
	config.rate.parameters.codec_tier.tier = 0;

	config.gop.keyframe_interval = ENC_KEYFRAME_LEADING_ONLY;
	config.conversion = ENC_CONVERSION_ALLOW;
	config.conversion = ENC_CONVERSION_FORBID;

	ENC_TEST_FATAL(enc_instance_new(&info, &instance));
	ENC_TEST_FATAL(enc_enumerate_devices(instance, &count, devices));

	for(index = 0; index < count; ++index) {
		struct enc_capabilities capabilities;
		struct enc_concurrency_capabilities concurrency;
		enum enc_surface_tier tier = ENC_SURFACE_TIER_NONE;

		if(!ENC_TEST_RESULT(enc_instance_query_capabilities(
				instance, index, ENC_BACKEND_NONE, ENC_CODEC_H264, &capabilities))) {

			preferred = capabilities.preferred_input_format;

			ENC_TEST_RESULT(enc_instance_query_format(
					instance, index, ENC_BACKEND_NONE, ENC_CODEC_H264, preferred, &tier));
		}

		ENC_TEST_RESULT(enc_instance_query_concurrency(
				instance, index, ENC_CODEC_H264, &concurrency));
	}

	enc_instance_delete(instance);

	return 0;
}

#include <encorder/encorder.h>

#include <encorder/test/common.h>

int main(void) {
	struct enc_instance_info info;
	struct enc_config config;
	struct enc_instance* instance = 0;
	struct enc_device_info devices[4];
	uint32_t count = sizeof(devices) / sizeof(devices[0]);

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

	ENC_TEST_FATAL(enc_instance_new(&info, &instance));
	ENC_TEST_FATAL(enc_enumerate_devices(instance, &count, devices));
	enc_instance_delete(instance);

	return 0;
}

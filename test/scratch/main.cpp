#include <encorder/encorder.h>

#include <encorder/test/common.h>

#include <vector>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

int main() {
	const auto version = enc_version();

	std::printf(
			"libencorder %u.%u.%u\n",
			ENC_VERSION_MAJOR(version),
			ENC_VERSION_MINOR(version),
			ENC_VERSION_PATCH(version));

	enc_instance_info info{};
	enc_instance_info_new(&info);

	info.application_name = "encorder-scratch";
	info.log = &enc_test_on_log;
	info.log_level = ENC_LOG_DEBUG;

	enc_instance* instance = nullptr;

	ENC_TEST_FATAL(enc_instance_new(&info, &instance));

	std::uint32_t count = 0;
	ENC_TEST_FATAL(enc_enumerate_devices(instance, &count, nullptr));

	std::vector<enc_device_info> devices(count);
	if(count) ENC_TEST_FATAL(enc_enumerate_devices(instance, &count, devices.data()));
	else {
		std::fprintf(stderr, "no encode capable devices\n");
		std::exit(EXIT_FAILURE);
	}

	std::printf("encoding devices:\n");

	for(std::uint32_t i = 0; i < count; ++i) {
		const auto& device = devices[i];

		std::printf("\t[%u] %s\n", i, device.name);
		std::printf(
				"\t\t- vendor: %04x\n"
				"\t\t- device: %04x\n"
				"\t\t- hardware: %s\n",
				device.vendor_id,
				device.device_id,
				device.is_hardware ? "yes" : "no");

		enc_test_dump_backends(device.backends, "\t\t");
		enc_test_dump_codecs(device.codecs, "\t\t");

		for(unsigned bit = 1; bit; bit <<= 1u) {
			const auto codec = static_cast<enc_codec>(device.codecs & bit);
			if(!codec) continue;

			std::printf("\t\t- %s:\n", enc_codec_name(codec));

			enc_capabilities capabilities{};

			if(ENC_TEST_RESULT(enc_instance_query_capabilities(
					instance, i, ENC_BACKEND_VULKAN, codec, &capabilities))) {

				continue;
			}

			std::printf(
					"\t\t\t- resolution: %ux%u .. %ux%u (align %ux%u)\n"
					"\t\t\t- references: %u (dpb %u), temporal layers: %u\n"
					"\t\t\t- quantizer: %u .. %u\n",
					capabilities.min_width,
					capabilities.min_height,
					capabilities.max_width,
					capabilities.max_height,
					capabilities.width_align,
					capabilities.height_align,
					capabilities.max_reference_frames,
					capabilities.max_retained_frames,
					capabilities.max_temporal_layers,
					capabilities.quantizer_min,
					capabilities.quantizer_max);

			std::printf(
					"\t\t\t- preferred input: %s\n",
					enc_format_name(capabilities.preferred_input_format));

			std::printf("\t\t\t- input formats:");

			for(std::uint32_t f = 0; f < capabilities.input_format_count; ++f) {
				enc_surface_tier tier = ENC_SURFACE_TIER_NONE;

				const auto format = capabilities.input_formats[f];

				enc_instance_query_format(
						instance, i, ENC_BACKEND_VULKAN, codec, format, &tier);

				std::printf(
						" %s(%s)",
						enc_format_name(format),
						tier == ENC_SURFACE_TIER_FUSED ? "fused"
								: tier == ENC_SURFACE_TIER_SPLIT ? "split"
								: tier == ENC_SURFACE_TIER_CONVERT ? "convert" : "?");
			}

			std::printf("\n");

			enc_concurrency_capabilities concurrency{};

			if(!ENC_TEST_RESULT(enc_instance_query_concurrency(
					instance, i, codec, &concurrency))) {

				std::printf(
						"\t\t\t- sessions: max %u (source %u), engines %u\n",
						concurrency.max_sessions,
						static_cast<unsigned>(concurrency.source),
						concurrency.encode_engine_count);
			}
		}
	}

	enc_instance_delete(instance);

	return EXIT_SUCCESS;
}

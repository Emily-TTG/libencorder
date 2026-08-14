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

		enc_device* opened = nullptr;
		if(ENC_TEST_RESULT(enc_device_new(instance, i, &opened))) continue;

		enc_concurrency_capabilities concurrency{};

		if(!ENC_TEST_RESULT(enc_device_query_concurrency(opened, ENC_CODEC_H264, &concurrency))) {
			std::printf(
					"\tsessions: max %u source %u\n",
					concurrency.max_sessions,
					static_cast<unsigned>(concurrency.source));
		}

		enc_device_delete(opened);
	}

	enc_instance_delete(instance);

	return EXIT_SUCCESS;
}

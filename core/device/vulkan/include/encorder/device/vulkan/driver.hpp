#pragma once

#include <encorder/core/library.hpp>
#include <encorder/core/device.hpp>

#include <volk.h>

namespace encorder::vulkan {
	struct queue_family {
		std::uint32_t index;
		VkQueueFlags flags;
		VkVideoCodecOperationFlagsKHR codec_operations;
	};

	struct physical_device_info {
		VkPhysicalDevice handle;

		enc_device_info info;

		std::uint32_t api_version;
		std::uint32_t driver_id;

		std::vector<std::string> extensions;
		std::vector<queue_family> families;

		std::optional<std::uint32_t> encode_family;
		std::optional<std::uint32_t> compute_family;

		enc_codec encode_codecs;
	};

	class driver final : public encorder::driver {
	private:
		const logger& log;

		shared_library library;
		VolkInstanceTable functions;

		VkInstance instance;

		std::vector<physical_device_info> physical_devices;
		std::vector<enc_device_info> device_infos;

	public:
		explicit driver(const logger&) noexcept;
		~driver() override;

		[[nodiscard]]
		result<void> initialise();

		[[nodiscard]]
		enc_native_kind native_kind() const noexcept override;

		[[nodiscard]]
		const char* name() const noexcept override;

		[[nodiscard]]
		std::span<const enc_device_info> devices() const noexcept override;

		[[nodiscard]]
		result<std::unique_ptr<device>> open(std::uint32_t) override;

	private:
		[[nodiscard]]
		result<void> load_library();

		[[nodiscard]]
		result<void> create_instance();

		[[nodiscard]]
		result<void> enumerate();

		[[nodiscard]]
		physical_device_info inspect(VkPhysicalDevice) const;
	};

	[[nodiscard]]
	result<std::unique_ptr<encorder::driver>> make_driver(const logger&);
}

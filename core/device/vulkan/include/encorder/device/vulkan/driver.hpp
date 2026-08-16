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

#define ENC_CHECK_VULKAN(expression, translate) check_vulkan(#expression, expression, translate)

#ifdef __GNUC__
# define ENC_VULKAN_STRUCT(...) \
		_Pragma("GCC diagnostic push") \
		_Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") \
		__VA_ARGS__ \
		_Pragma("GCC diagnostic pop")
#else
# define ENC_VULKAN_STRUCT(...) __VA_ARGS__
#endif

	[[nodiscard]]
	std::expected<void, error> check_vulkan(std::string_view, VkResult, enc_result);

	[[nodiscard]]
	result<std::unique_ptr<encorder::driver>> make_driver(const logger&);
}

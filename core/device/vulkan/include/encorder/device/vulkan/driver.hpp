#pragma once

#include <encorder/core/library.hpp>
#include <encorder/core/device.hpp>
#include <encorder/device/vulkan/common.hpp>
#include <encorder/device/vulkan/functions.hpp>


namespace encorder::vulkan {
	struct api_codec {
		VkVideoCodecOperationFlagBitsKHR flag;
		enc_codec codec;
		const char* extension;
	};

	inline constexpr std::array codec_mappings{
			api_codec{
					.flag = VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR,
					.codec = ENC_CODEC_H264,
					.extension = VK_KHR_VIDEO_ENCODE_H264_EXTENSION_NAME
			},
			api_codec{
					.flag = VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR,
					.codec = ENC_CODEC_HEVC,
					.extension = VK_KHR_VIDEO_ENCODE_H265_EXTENSION_NAME
			},
			api_codec{
					.flag = VK_VIDEO_CODEC_OPERATION_ENCODE_AV1_BIT_KHR,
					.codec = ENC_CODEC_AV1,
					.extension = VK_KHR_VIDEO_ENCODE_AV1_EXTENSION_NAME
			}
	};

	struct queue_family {
		std::uint32_t index;
		std::uint32_t queue_count;
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

		loader_functions loader;
		instance_functions functions;

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

		[[nodiscard]]
		result<std::unique_ptr<device>> adopt(const void*) override;

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

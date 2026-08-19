#pragma once

#include <encorder/device/vulkan/driver.hpp>
#include <encorder/device/vulkan/functions.hpp>
#include <encorder/device/vulkan/profile.hpp>

namespace encorder::vulkan {
	struct codec_capabilities {
		enc_capabilities capabilities;

		// The cell the numeric limits above were measured on.
		profile_key primary;

		// Every (chroma, depth) the device accepted for this codec.
		std::vector<profile_key> profiles;

		VkExtensionProperties std_header_version;
		VkVideoCapabilityFlagsKHR capability_flags;

		VkDeviceSize min_bitstream_buffer_offset_alignment;
		VkDeviceSize min_bitstream_buffer_size_alignment;

		std::uint64_t max_bitrate;
		std::uint32_t max_quality_levels;
		std::uint32_t max_dpb_slots;
		std::uint32_t max_active_references;

		VkVideoEncodeFeedbackFlagsKHR supported_feedback;

		VkFormat picture_format;
		VkFormat reference_format;
	};

	[[nodiscard]]
	result<codec_capabilities> query_codec_capabilities(
			const instance_functions&,
			const physical_device_info&,
			enc_codec,
			const logger&);

	[[nodiscard]]
	result<enc_surface_tier> query_format_tier(
			const instance_functions&,
			const physical_device_info&,
			const codec_capabilities&,
			enc_format);
}

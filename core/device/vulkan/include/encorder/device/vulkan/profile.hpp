#pragma once

#include <encorder/device/vulkan/common.hpp>

namespace encorder::vulkan {
	struct profile_key {
		enc_codec codec;

		VkVideoChromaSubsamplingFlagBitsKHR chroma;
		VkVideoComponentBitDepthFlagBitsKHR luma_depth;

		// `INVALID` for monochrome.
		VkVideoComponentBitDepthFlagBitsKHR chroma_depth;

		[[nodiscard]]
		bool operator==(const profile_key&) const noexcept = default;
	};

	class profile_chain {
	private:
		profile_key key;

		union codec_profile {
			VkVideoEncodeH264ProfileInfoKHR h264;
			VkVideoEncodeH265ProfileInfoKHR h265;
			VkVideoEncodeAV1ProfileInfoKHR av1;
		};

		codec_profile codec;
		VkVideoEncodeUsageInfoKHR usage;
		VkVideoProfileInfoKHR profile;
		VkVideoProfileListInfoKHR list;

	public:
		explicit profile_chain(const profile_key&) noexcept;

		profile_chain(const profile_chain&) = delete;
		profile_chain& operator=(const profile_chain&) = delete;

		profile_chain(profile_chain&&) = delete;
		profile_chain& operator=(profile_chain&&) = delete;

	public:
		[[nodiscard]]
		const profile_key& describes() const noexcept;

		[[nodiscard]]
		const VkVideoProfileInfoKHR& info() const noexcept;

		// For the `pNext` of image, buffer and format-property creation info.
		[[nodiscard]]
		const VkVideoProfileListInfoKHR& profile_list() const noexcept;
	};

	[[nodiscard]]
	VkVideoCodecOperationFlagBitsKHR codec_operation(enc_codec) noexcept;

	[[nodiscard]]
	enc_chroma to_enc_chroma(VkVideoChromaSubsamplingFlagBitsKHR) noexcept;

	/* 0 when the flag names no single depth. */
	[[nodiscard]]
	std::uint32_t depth_bits(VkVideoComponentBitDepthFlagBitsKHR) noexcept;
}

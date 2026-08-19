#include <encorder/device/vulkan/profile.hpp>

namespace encorder::vulkan {
	namespace {
		[[nodiscard]]
		StdVideoH264ProfileIdc h264_profile(const profile_key& key) noexcept {
			if(key.chroma == VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR ||
					key.chroma == VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR) {

				return STD_VIDEO_H264_PROFILE_IDC_HIGH_444_PREDICTIVE;
			}

			if(key.chroma == VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR) {
				return STD_VIDEO_H264_PROFILE_IDC_HIGH_422;
			}

			if(key.luma_depth != VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR) {
				return STD_VIDEO_H264_PROFILE_IDC_HIGH_10;
			}

			return STD_VIDEO_H264_PROFILE_IDC_HIGH;
		}

		[[nodiscard]]
		StdVideoH265ProfileIdc h265_profile(const profile_key& key) noexcept {
			if(key.chroma == VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR) {
				return STD_VIDEO_H265_PROFILE_IDC_FORMAT_RANGE_EXTENSIONS;
			}

			if(key.chroma == VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR) {
				return STD_VIDEO_H265_PROFILE_IDC_FORMAT_RANGE_EXTENSIONS;
			}

			if(key.luma_depth == VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR) {
				return STD_VIDEO_H265_PROFILE_IDC_MAIN;
			}

			if(key.luma_depth == VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR) {
				return STD_VIDEO_H265_PROFILE_IDC_MAIN_10;
			}

			return STD_VIDEO_H265_PROFILE_IDC_FORMAT_RANGE_EXTENSIONS;
		}

		[[nodiscard]]
		StdVideoAV1Profile av1_profile(const profile_key& key) noexcept {
			if(key.chroma == VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR) {
				return STD_VIDEO_AV1_PROFILE_HIGH;
			}

			if(key.chroma == VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR ||
					key.luma_depth == VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR) {

				return STD_VIDEO_AV1_PROFILE_PROFESSIONAL;
			}

			return STD_VIDEO_AV1_PROFILE_MAIN;
		}
	}

	profile_chain::profile_chain(const profile_key& source) noexcept :
			key(source),
			codec{},
			usage{},
			profile{},
			list{} {

		const void* codec_next = nullptr;

		switch(key.codec) {
			case ENC_CODEC_H264: {
				ENC_VULKAN_STRUCT(codec.h264 = VkVideoEncodeH264ProfileInfoKHR{
						.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR,
						.stdProfileIdc = h264_profile(key)
				};)

				codec_next = &codec.h264;
				break;
			}

			case ENC_CODEC_HEVC: {
				ENC_VULKAN_STRUCT(codec.h265 = VkVideoEncodeH265ProfileInfoKHR{
						.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_KHR,
						.stdProfileIdc = h265_profile(key)
				};)

				codec_next = &codec.h265;
				break;
			}

			case ENC_CODEC_AV1: {
				ENC_VULKAN_STRUCT(codec.av1 = VkVideoEncodeAV1ProfileInfoKHR{
						.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_PROFILE_INFO_KHR,
						.stdProfile = av1_profile(key)
				};)

				codec_next = &codec.av1;
				break;
			}

			default: break;
		}

		ENC_VULKAN_STRUCT(usage = VkVideoEncodeUsageInfoKHR{
				.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR,
				.pNext = codec_next,
				.videoUsageHints = VK_VIDEO_ENCODE_USAGE_STREAMING_BIT_KHR,
				.videoContentHints = VK_VIDEO_ENCODE_CONTENT_RENDERED_BIT_KHR,
				.tuningMode = VK_VIDEO_ENCODE_TUNING_MODE_DEFAULT_KHR
		};)

		ENC_VULKAN_STRUCT(profile = VkVideoProfileInfoKHR{
				.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR,
				.pNext = &usage,
				.videoCodecOperation = codec_operation(key.codec),
				.chromaSubsampling = static_cast<VkVideoChromaSubsamplingFlagsKHR>(key.chroma),
				.lumaBitDepth = static_cast<VkVideoComponentBitDepthFlagsKHR>(key.luma_depth),
				.chromaBitDepth = static_cast<VkVideoComponentBitDepthFlagsKHR>(key.chroma_depth)
		};)

		ENC_VULKAN_STRUCT(list = VkVideoProfileListInfoKHR{
				.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR,
				.profileCount = 1,
				.pProfiles = &profile
		};)
	}

	const profile_key& profile_chain::describes() const noexcept {
		return key;
	}

	const VkVideoProfileInfoKHR& profile_chain::info() const noexcept {
		return profile;
	}

	const VkVideoProfileListInfoKHR& profile_chain::profile_list() const noexcept {
		return list;
	}

	VkVideoCodecOperationFlagBitsKHR codec_operation(const enc_codec codec) noexcept {
		switch(codec) {
			case ENC_CODEC_H264: return VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR;
			case ENC_CODEC_HEVC: return VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR;
			case ENC_CODEC_AV1: return VK_VIDEO_CODEC_OPERATION_ENCODE_AV1_BIT_KHR;
			default: return VK_VIDEO_CODEC_OPERATION_NONE_KHR;
		}
	}

	enc_chroma to_enc_chroma(const VkVideoChromaSubsamplingFlagBitsKHR chroma) noexcept {
		switch(chroma) {
			case VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR: return ENC_CHROMA_MONOCHROME;
			case VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR: return ENC_CHROMA_420;
			case VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR: return ENC_CHROMA_422;
			case VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR: return ENC_CHROMA_444;
			default: return ENC_CHROMA_NONE;
		}
	}

	std::uint32_t depth_bits(const VkVideoComponentBitDepthFlagBitsKHR depth) noexcept {
		switch(depth) {
			case VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR: return 8;
			case VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR: return 10;
			case VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR: return 12;
			default: return 0;
		}
	}
}

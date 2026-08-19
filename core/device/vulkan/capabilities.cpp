#include <encorder/device/vulkan/capabilities.hpp>
#include <encorder/device/vulkan/formats.hpp>

#include <encorder/core/error.inl>
#include <encorder/core/logger.inl>
#include <encorder/core/format.hpp>

using namespace magic_enum::bitwise_operators;

namespace encorder::vulkan {
	namespace {
		constexpr std::array probe_chromas{
				VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR,
				VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR,
				VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR,
				VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR
		};

		constexpr std::array probe_depths{
				VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR,
				VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR,
				VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR
		};

		[[nodiscard]]
		bool unsupported_profile(const VkResult status) noexcept {
			switch(status) {
				case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: [[fallthrough]];
				case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: [[fallthrough]];
				case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: [[fallthrough]];
				case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: [[fallthrough]];
				case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: [[fallthrough]];
				case VK_ERROR_FORMAT_NOT_SUPPORTED: return true;

				default: return false;
			}
		}

		[[nodiscard]]
		profile_key make_key(
				const enc_codec codec,
				const VkVideoChromaSubsamplingFlagBitsKHR chroma,
				const VkVideoComponentBitDepthFlagBitsKHR depth) noexcept {

			return profile_key{
					.codec = codec,
					.chroma = chroma,
					.luma_depth = depth,
					.chroma_depth = chroma == VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR
							? VK_VIDEO_COMPONENT_BIT_DEPTH_INVALID_KHR
							: depth
			};
		}

		// TODO(Emily): This is kinda chunky because I didn't want to separate it atm.
		class capability_probe {
		private:
			union {
				VkVideoEncodeH264CapabilitiesKHR h264;
				VkVideoEncodeH265CapabilitiesKHR h265;
				VkVideoEncodeAV1CapabilitiesKHR av1;
			} codec;

			VkVideoEncodeCapabilitiesKHR encode;
			VkVideoCapabilitiesKHR video;

			enc_codec kind;

		public:
			explicit capability_probe(const enc_codec source) noexcept :
					codec{},
					encode{},
					video{},
					kind(source) {

				void* codec_next = nullptr;

				switch(kind) {
					case ENC_CODEC_H264: {
						ENC_VULKAN_STRUCT(codec.h264 = VkVideoEncodeH264CapabilitiesKHR{
								.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_KHR
						};)

						codec_next = &codec.h264;
						break;
					}

					case ENC_CODEC_HEVC: {
						ENC_VULKAN_STRUCT(codec.h265 = VkVideoEncodeH265CapabilitiesKHR{
								.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_CAPABILITIES_KHR
						};)

						codec_next = &codec.h265;
						break;
					}

					case ENC_CODEC_AV1: {
						ENC_VULKAN_STRUCT(codec.av1 = VkVideoEncodeAV1CapabilitiesKHR{
								.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_CAPABILITIES_KHR
						};)

						codec_next = &codec.av1;
						break;
					}

					default: break;
				}

				ENC_VULKAN_STRUCT(encode = VkVideoEncodeCapabilitiesKHR{
						.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR,
						.pNext = codec_next
				};)

				ENC_VULKAN_STRUCT(video = VkVideoCapabilitiesKHR{
						.sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR,
						.pNext = &encode
				};)
			}

			capability_probe(const capability_probe&) = delete;
			capability_probe& operator=(const capability_probe&) = delete;

			capability_probe(capability_probe&&) = delete;
			capability_probe& operator=(capability_probe&&) = delete;

		public:
			[[nodiscard]]
			VkResult run(
					const instance_functions& functions,
					const VkPhysicalDevice handle,
					const profile_chain& chain) noexcept {

				return functions.vkGetPhysicalDeviceVideoCapabilitiesKHR(
						handle,
						&chain.info(),
						&video);
			}

			[[nodiscard]]
			const VkVideoCapabilitiesKHR& common() const noexcept { return video; }

			[[nodiscard]]
			const VkVideoEncodeCapabilitiesKHR& encoding() const noexcept { return encode; }

			[[nodiscard]]
			const VkVideoEncodeH264CapabilitiesKHR& h264() const noexcept { return codec.h264; }

			[[nodiscard]]
			const VkVideoEncodeH265CapabilitiesKHR& h265() const noexcept { return codec.h265; }

			[[nodiscard]]
			const VkVideoEncodeAV1CapabilitiesKHR& av1() const noexcept { return codec.av1; }
		};

		[[nodiscard]]
		result<std::vector<VkVideoFormatPropertiesKHR>> query_video_formats(
				const instance_functions& functions,
				const VkPhysicalDevice handle,
				const profile_chain& chain,
				const VkImageUsageFlags usage) {

			ENC_VULKAN_STRUCT(const VkPhysicalDeviceVideoFormatInfoKHR request{
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_FORMAT_INFO_KHR,
					.pNext = &chain.profile_list(),
					.imageUsage = usage
			};)

			std::uint32_t count = 0;

			if(const auto status = ENC_CHECK_VULKAN(
					functions.vkGetPhysicalDeviceVideoFormatPropertiesKHR(handle, &request, &count, nullptr),
					ENC_RESULT_ERROR_FORMAT_UNSUPPORTED); !status) {

				return std::unexpected(status.error());
			}

			std::vector<VkVideoFormatPropertiesKHR> properties(count);

			for(auto& entry : properties) {
				entry = VkVideoFormatPropertiesKHR{
						.sType = VK_STRUCTURE_TYPE_VIDEO_FORMAT_PROPERTIES_KHR,
						.pNext = nullptr,
						.format = VK_FORMAT_UNDEFINED,
						.componentMapping = {},
						.imageCreateFlags = 0,
						.imageType = VK_IMAGE_TYPE_2D,
						.imageTiling = VK_IMAGE_TILING_OPTIMAL,
						.imageUsageFlags = 0
				};
			}

			if(!count) return properties;

			if(const auto status = ENC_CHECK_VULKAN(
					functions.vkGetPhysicalDeviceVideoFormatPropertiesKHR(handle, &request, &count, properties.data()),
					ENC_RESULT_ERROR_FORMAT_UNSUPPORTED); !status) {

				return std::unexpected(status.error());
			}

			properties.resize(std::min<std::size_t>(count, properties.size()));

			return properties;
		}

		[[nodiscard]]
		enc_surface_tier tier_of(const VkImageUsageFlags usage) noexcept {
			if(!(usage & VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR)) return ENC_SURFACE_TIER_NONE;

			if(usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT)) {
				return ENC_SURFACE_TIER_FUSED;
			}

			return ENC_SURFACE_TIER_SPLIT;
		}

		[[nodiscard]]
		enc_rate_mode rate_modes_of(const VkVideoEncodeCapabilitiesKHR& encode) noexcept {
			auto modes = ENC_RATE_NONE;

			if(encode.rateControlModes & VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DISABLED_BIT_KHR) {
				modes = modes | ENC_RATE_CONSTANT_QUANTISER;
			}

			if(encode.rateControlModes & VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR) {
				modes = modes | ENC_RATE_CONSTANT_BITRATE;
			}

			if(encode.rateControlModes & VK_VIDEO_ENCODE_RATE_CONTROL_MODE_VBR_BIT_KHR) {
				modes = modes | ENC_RATE_AVERAGE_BITRATE;

				if(encode.maxQualityLevels > 1) modes = modes | ENC_RATE_CONSTANT_QUALITY;
			}

			return modes;
		}

		struct codec_limits {
			std::uint32_t max_references;
			std::uint32_t max_temporal_layers;
			std::uint32_t quantizer_min;
			std::uint32_t quantizer_max;
			bool b_frames;
		};

		[[nodiscard]]
		codec_limits limits_of(const capability_probe& probe, const enc_codec codec) noexcept {
			switch(codec) {
				case ENC_CODEC_H264: {
					const auto& caps = probe.h264();

					return codec_limits{
							.max_references = caps.maxPPictureL0ReferenceCount + caps.maxL1ReferenceCount,
							.max_temporal_layers = caps.maxTemporalLayerCount,
							.quantizer_min = static_cast<std::uint32_t>(std::max(caps.minQp, 0)),
							.quantizer_max = static_cast<std::uint32_t>(std::max(caps.maxQp, 0)),
							.b_frames = caps.maxBPictureL0ReferenceCount > 0 || caps.maxL1ReferenceCount > 0
					};
				}

				case ENC_CODEC_HEVC: {
					const auto& caps = probe.h265();

					return codec_limits{
							.max_references = caps.maxPPictureL0ReferenceCount + caps.maxL1ReferenceCount,
							.max_temporal_layers = caps.maxSubLayerCount,
							.quantizer_min = static_cast<std::uint32_t>(std::max(caps.minQp, 0)),
							.quantizer_max = static_cast<std::uint32_t>(std::max(caps.maxQp, 0)),
							.b_frames = caps.maxBPictureL0ReferenceCount > 0 || caps.maxL1ReferenceCount > 0
					};
				}

				case ENC_CODEC_AV1: {
					const auto& caps = probe.av1();

					return codec_limits{
							.max_references = caps.maxSingleReferenceCount + caps.maxBidirectionalCompoundReferenceCount,
							.max_temporal_layers = caps.maxTemporalLayerCount,
							.quantizer_min = caps.minQIndex,
							.quantizer_max = caps.maxQIndex,
							.b_frames = caps.maxBidirectionalCompoundReferenceCount > 0
					};
				}

				default: return codec_limits{};
			}
		}
	}

	result<codec_capabilities> query_codec_capabilities(
			const instance_functions& functions,
			const physical_device_info& physical,
			const enc_codec codec,
			const logger& log) {

		if(!functions.vkGetPhysicalDeviceVideoCapabilitiesKHR ||
				!functions.vkGetPhysicalDeviceVideoFormatPropertiesKHR) {

			return unexpect(
					ENC_RESULT_ERROR_UNSUPPORTED,
					"`{}` is missing video capability queries",
					physical.info.name);
		}

		if(!(physical.encode_codecs & codec)) {
			return unexpect(
					ENC_RESULT_ERROR_CODEC_UNSUPPORTED,
					"`{}` does not encode `{}`, only `{}`",
					physical.info.name,
					enc_codec_name(codec),
					magic_enum::enum_flags_name<enc_codec>(physical.encode_codecs));
		}

		auto chromas = ENC_CHROMA_NONE;
		auto depths = ENC_BIT_DEPTH_NONE;

		std::optional<profile_key> primary{};
		std::vector<profile_key> supported{};

		for(const auto chroma : probe_chromas) {
			for(const auto depth : probe_depths) {
				const auto key = make_key(codec, chroma, depth);

				const profile_chain chain(key);
				capability_probe probe(codec);

				const auto status = probe.run(functions, physical.handle, chain);

				if(status != VK_SUCCESS) {
					if(!unsupported_profile(status)) {
						log.log(
								ENC_LOG_DEBUG,
								"vulkan: `{}` capability probe for `{}` {}/{}-bit failed: `{}`",
								physical.info.name,
								enc_codec_name(codec),
								magic_enum::enum_name<enc_chroma>(to_enc_chroma(chroma)),
								depth_bits(depth),
								magic_enum::enum_name<VkResult>(status));
					}

					continue;
				}

				chromas = chromas | to_enc_chroma(chroma);
				depths = depths | bit_depth_flag(depth_bits(depth));

				supported.push_back(key);

				if(!primary || (chroma == VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR && depth == VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR)) {
					primary = key;
				}
			}
		}

		if(!primary) {
			return unexpect(
					ENC_RESULT_ERROR_CODEC_UNSUPPORTED,
					"`{}` advertises `{}` but supports no chroma/depth combination",
					physical.info.name,
					enc_codec_name(codec));
		}

		const profile_chain chain(*primary);
		capability_probe probe(codec);

		if(const auto status = ENC_CHECK_VULKAN(
				probe.run(functions, physical.handle, chain),
				ENC_RESULT_ERROR_CODEC_UNSUPPORTED); !status) {

			return std::unexpected(status.error());
		}

		const auto& common = probe.common();
		const auto& encoding = probe.encoding();
		const auto limits = limits_of(probe, codec);

		codec_capabilities result{};

		result.primary = *primary;
		result.profiles = supported;
		result.std_header_version = common.stdHeaderVersion;
		result.capability_flags = common.flags;
		result.min_bitstream_buffer_offset_alignment = common.minBitstreamBufferOffsetAlignment;
		result.min_bitstream_buffer_size_alignment = common.minBitstreamBufferSizeAlignment;
		result.max_bitrate = encoding.maxBitrate;
		result.max_quality_levels = encoding.maxQualityLevels;
		result.max_dpb_slots = common.maxDpbSlots;
		result.max_active_references = common.maxActiveReferencePictures;
		result.supported_feedback = encoding.supportedEncodeFeedbackFlags;

		auto& capabilities = result.capabilities;

		capabilities.struct_size = sizeof(enc_capabilities);
		capabilities.version = 0;

		capabilities.backend = ENC_BACKEND_VULKAN;
		capabilities.codec = codec;
		capabilities.is_hardware = physical.info.is_hardware;

		// Vulkan encode reads a `VkImage` directly.
		capabilities.host_access = false;

		capabilities.min_width = common.minCodedExtent.width;
		capabilities.min_height = common.minCodedExtent.height;
		capabilities.max_width = common.maxCodedExtent.width;
		capabilities.max_height = common.maxCodedExtent.height;

		capabilities.width_align = std::max(
				common.pictureAccessGranularity.width,
				encoding.encodeInputPictureGranularity.width);

		capabilities.height_align = std::max(
				common.pictureAccessGranularity.height,
				encoding.encodeInputPictureGranularity.height);

		capabilities.max_reference_frames = std::min(
				common.maxActiveReferencePictures,
				limits.max_references);

		capabilities.max_temporal_layers = std::min(
				std::max(encoding.maxRateControlLayers, 1u),
				std::max(limits.max_temporal_layers, 1u));

		capabilities.max_retained_frames = common.maxDpbSlots;

		capabilities.quantizer_min = limits.quantizer_min;
		capabilities.quantizer_max = limits.quantizer_max;

		capabilities.rate_modes = rate_modes_of(encoding);
		capabilities.chroma_formats = chromas;
		capabilities.bit_depths = depths;

		auto features = ENC_FEATURE_NONE;

		if(limits.b_frames) features = features | ENC_FEATURE_B_FRAMES;
		if(capabilities.max_temporal_layers > 1) features = features | ENC_FEATURE_TEMPORAL_LAYERS;

		if(encoding.rateControlModes != VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DEFAULT_KHR) {
			features = features | ENC_FEATURE_DYNAMIC_BITRATE;
		}

		if(encoding.flags & VK_VIDEO_ENCODE_CAPABILITY_QUANTIZATION_DELTA_MAP_BIT_KHR) {
			features = features | ENC_FEATURE_ROI_QP;
		}

		capabilities.features = features;

		auto tiers = ENC_SURFACE_TIER_NONE;
		auto best_tier = ENC_SURFACE_TIER_NONE;
		auto truncated = false;

		for(const auto& key : supported) {
			const profile_chain cell(key);

			auto sources = query_video_formats(
					functions,
					physical.handle,
					cell,
					VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR);

			if(!sources) continue;

			for(const auto& entry : *sources) {
				const auto mapped = to_enc_format(entry.format);
				const auto tier = tier_of(entry.imageUsageFlags);

				tiers = tiers | tier;

				if(mapped == ENC_FORMAT_UNDEFINED) {
					log.log(
							ENC_LOG_DEBUG,
							"vulkan: `{}` encode source format `{}` has no `enc_format`",
							physical.info.name,
							magic_enum::enum_name<VkFormat>(entry.format));

					continue;
				}

				const auto listed = std::span(
						capabilities.input_formats,
						capabilities.input_format_count);

				if(std::ranges::contains(listed, mapped)) continue;

				if(capabilities.input_format_count < std::size(capabilities.input_formats)) {
					capabilities.input_formats[capabilities.input_format_count++] = mapped;
				} else {
					truncated = true;
				}

				// Prefer a format needing no staging, otherwise the first usable one.
				if(capabilities.preferred_input_format == ENC_FORMAT_UNDEFINED ||
						(tier == ENC_SURFACE_TIER_FUSED && best_tier != ENC_SURFACE_TIER_FUSED)) {

					capabilities.preferred_input_format = mapped;
					best_tier = tier;
				}
			}
		}

		if(truncated) {
			log.log(
					ENC_LOG_WARN,
					"vulkan: `{}` `{}` supports more than {} input formats; list truncated",
					physical.info.name,
					enc_codec_name(codec),
					std::size(capabilities.input_formats));
		}

		if(!capabilities.input_format_count) {
			return unexpect(
					ENC_RESULT_ERROR_FORMAT_UNSUPPORTED,
					"`{}` reports no usable encode source format for `{}`",
					physical.info.name,
					enc_codec_name(codec));
		}

		auto sources = query_video_formats(
				functions,
				physical.handle,
				chain,
				VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR);

		if(!sources) return std::unexpected(sources.error());

		auto references = query_video_formats(
				functions,
				physical.handle,
				chain,
				VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR);

		if(!references) return std::unexpected(references.error());

		if(sources->empty()) {
			return unexpect(
					ENC_RESULT_ERROR_FORMAT_UNSUPPORTED,
					"`{}` reports no encode source format for `{}` at the primary profile",
					physical.info.name,
					enc_codec_name(codec));
		}

		if(physical.compute_family) tiers = tiers | ENC_SURFACE_TIER_CONVERT;

		capabilities.surface_tiers = tiers;

		result.picture_format = sources->front().format;
		result.reference_format = references->empty()
				? sources->front().format
				: references->front().format;

		log.log(
				ENC_LOG_DEBUG,
				"vulkan: `{}` `{}` {}x{}..{}x{}; chroma `{}`; depths `{}`; rate `{}`; {} input format{}",
				physical.info.name,
				enc_codec_name(codec),
				capabilities.min_width,
				capabilities.min_height,
				capabilities.max_width,
				capabilities.max_height,
				magic_enum::enum_flags_name<enc_chroma>(capabilities.chroma_formats),
				magic_enum::enum_flags_name<enc_bit_depth>(capabilities.bit_depths),
				magic_enum::enum_flags_name<enc_rate_mode>(capabilities.rate_modes),
				capabilities.input_format_count,
				capabilities.input_format_count != 1 ? "s" : "");

		return result;
	}

	result<enc_surface_tier> query_format_tier(
			const instance_functions& functions,
			const physical_device_info& physical,
			const codec_capabilities& queried,
			const enc_format format) {

		const auto native = to_vulkan_format(format);

		if(!native) {
			return unexpect(
					ENC_RESULT_ERROR_FORMAT_UNSUPPORTED,
					"`{}` has no vulkan equivalent",
					enc_format_name(format));
		}

		auto best = ENC_SURFACE_TIER_NONE;

		for(const auto& key : queried.profiles) {
			const profile_chain cell(key);

			auto sources = query_video_formats(
					functions,
					physical.handle,
					cell,
					VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR);

			if(!sources) continue;

			for(const auto& entry : *sources) {
				if(entry.format != *native) continue;

				const auto tier = tier_of(entry.imageUsageFlags);

				if(tier == ENC_SURFACE_TIER_FUSED) return tier;

				if(tier != ENC_SURFACE_TIER_NONE) best = tier;
			}
		}

		if(best != ENC_SURFACE_TIER_NONE) return best;

		if(physical.compute_family) return ENC_SURFACE_TIER_CONVERT;

		return unexpect(
				ENC_RESULT_ERROR_FORMAT_UNSUPPORTED,
				"`{}` is not an encode source for `{}` and no compute queue could convert it",
				enc_format_name(format),
				enc_codec_name(queried.capabilities.codec));
	}
}

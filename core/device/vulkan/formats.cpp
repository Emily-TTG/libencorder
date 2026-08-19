#include <encorder/device/vulkan/formats.hpp>

namespace encorder::vulkan {
	namespace {
		/*
		 * Only formats a Vulkan Video encoder can consume, plus the formats the
		 * conversion can take.
		 *
		 * AYUV, Y410 and Y416 have no `VkFormat`. Packed RGBA formats component
		 * order would be wrong.
		 */
		// TODO(Emily): `VK_FORMAT_G12X4_*_2PLANE_422/444` have no `enc_format` yet (P212/P412).
		constexpr native_format_map format_map{std::array{
				format_mapping<VkFormat>{ ENC_FORMAT_NV12, VK_FORMAT_G8_B8R8_2PLANE_420_UNORM },
				format_mapping<VkFormat>{ ENC_FORMAT_P010, VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 },
				format_mapping<VkFormat>{ ENC_FORMAT_P012, VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 },
				format_mapping<VkFormat>{ ENC_FORMAT_P016, VK_FORMAT_G16_B16R16_2PLANE_420_UNORM },

				format_mapping<VkFormat>{ ENC_FORMAT_NV16, VK_FORMAT_G8_B8R8_2PLANE_422_UNORM },
				format_mapping<VkFormat>{ ENC_FORMAT_P210, VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 },
				format_mapping<VkFormat>{ ENC_FORMAT_P216, VK_FORMAT_G16_B16R16_2PLANE_422_UNORM },
				format_mapping<VkFormat>{ ENC_FORMAT_Y210, VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 },

				format_mapping<VkFormat>{ ENC_FORMAT_NV24, VK_FORMAT_G8_B8R8_2PLANE_444_UNORM },
				format_mapping<VkFormat>{ ENC_FORMAT_P410, VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16 },
				format_mapping<VkFormat>{ ENC_FORMAT_P416, VK_FORMAT_G16_B16R16_2PLANE_444_UNORM },

				format_mapping<VkFormat>{ ENC_FORMAT_RGBA8, VK_FORMAT_R8G8B8A8_UNORM },
				format_mapping<VkFormat>{ ENC_FORMAT_BGRA8, VK_FORMAT_B8G8R8A8_UNORM },
				format_mapping<VkFormat>{ ENC_FORMAT_RGB10A2, VK_FORMAT_A2B10G10R10_UNORM_PACK32 },
				format_mapping<VkFormat>{ ENC_FORMAT_RGBA16F, VK_FORMAT_R16G16B16A16_SFLOAT }
		}};
	}

	enc_format to_enc_format(const VkFormat format) noexcept {
		return format_map.from_native(format);
	}

	std::optional<VkFormat> to_vulkan_format(const enc_format format) noexcept {
		return format_map.to_native(format);
	}
}

#pragma once

#include <encorder/core/format_map.hpp>

#include <encorder/device/vulkan/common.hpp>

namespace encorder::vulkan {
	// `ENC_FORMAT_UNDEFINED` when the format has no `enc_format`.
	[[nodiscard]]
	enc_format to_enc_format(VkFormat) noexcept;

	[[nodiscard]]
	std::optional<VkFormat> to_vulkan_format(enc_format) noexcept;
}

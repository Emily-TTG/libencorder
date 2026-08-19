#include <encorder/device/vulkan/common.hpp>

#include <encorder/core/error.inl>

namespace encorder::vulkan {
	std::expected<void, error> check_vulkan(
			const std::string_view context,
			const VkResult vulkan_result,
			const enc_result result) {

		if(vulkan_result == VK_SUCCESS) return {};

		return unexpect(
				result,
				"`{}`: `{}`",
				context,
				magic_enum::enum_name<VkResult>(vulkan_result));
	}
}

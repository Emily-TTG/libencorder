#pragma once

#include <encorder/core/error.hpp>

#ifndef VK_NO_PROTOTYPES
# define VK_NO_PROTOTYPES
#endif

#include <vulkan/vulkan.h>

namespace encorder::vulkan {
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
}

#include <encorder/core/format.hpp>

namespace encorder {
	namespace {
		template<auto format_info::* member>
		[[nodiscard]]
		const std::decay_t<std::invoke_result_t<decltype(member), format_info>>& lookup(
				const enc_format format,
				const std::decay_t<std::invoke_result_t<decltype(member), format_info>>& fallback) noexcept {

			const auto* const match = find_format(format);

			return match ? match->*member : fallback;
		}
	}

	ENC_API const char* enc_format_name(const enc_format format) {
		return lookup<&format_info::name>(format, "undefined");
	}

	ENC_API enc_chroma enc_format_chroma(const enc_format format) {
		return lookup<&format_info::chroma>(format, ENC_CHROMA_NONE);
	}

	ENC_API uint32_t enc_format_bit_depth(const enc_format format) {
		return lookup<&format_info::bit_depth>(format, 0);
	}

	ENC_API uint32_t enc_format_plane_count(const enc_format format) {
		return lookup<&format_info::plane_count>(format, 0);
	}

	ENC_API bool enc_format_is_rgb(const enc_format format) {
		return lookup<&format_info::rgb>(format, false);
	}
}

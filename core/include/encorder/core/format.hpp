#pragma once

namespace encorder {
	struct format_info {
		enc_format format;
		const char* name;
		enc_chroma chroma;
		std::uint32_t bit_depth;
		std::uint32_t plane_count;
		bool rgb;
	};

	/* TODO(Emily): Move to inl. */
	inline constexpr std::array format_table{
			format_info{ .format = ENC_FORMAT_NV12, .name = "NV12", .chroma = ENC_CHROMA_420, .bit_depth = 8, .plane_count = 2, .rgb = false },
			format_info{ .format = ENC_FORMAT_P010, .name = "P010", .chroma = ENC_CHROMA_420, .bit_depth = 10, .plane_count = 2, .rgb = false },
			format_info{ .format = ENC_FORMAT_P012, .name = "P012", .chroma = ENC_CHROMA_420, .bit_depth = 12, .plane_count = 2, .rgb = false },
			format_info{ .format = ENC_FORMAT_P016, .name = "P016", .chroma = ENC_CHROMA_420, .bit_depth = 16, .plane_count = 2, .rgb = false },

			format_info{ .format = ENC_FORMAT_NV16, .name = "NV16", .chroma = ENC_CHROMA_422, .bit_depth = 8, .plane_count = 2, .rgb = false },
			format_info{ .format = ENC_FORMAT_P210, .name = "P210", .chroma = ENC_CHROMA_422, .bit_depth = 10, .plane_count = 2, .rgb = false },
			format_info{ .format = ENC_FORMAT_P216, .name = "P216", .chroma = ENC_CHROMA_422, .bit_depth = 16, .plane_count = 2, .rgb = false },
			format_info{ .format = ENC_FORMAT_Y210, .name = "Y210", .chroma = ENC_CHROMA_422, .bit_depth = 10, .plane_count = 1, .rgb = false },

			format_info{ .format = ENC_FORMAT_NV24, .name = "NV24", .chroma = ENC_CHROMA_444, .bit_depth = 8, .plane_count = 2, .rgb = false },
			format_info{ .format = ENC_FORMAT_P410, .name = "P410", .chroma = ENC_CHROMA_444, .bit_depth = 10, .plane_count = 2, .rgb = false },
			format_info{ .format = ENC_FORMAT_P416, .name = "P416", .chroma = ENC_CHROMA_444, .bit_depth = 16, .plane_count = 2, .rgb = false },
			format_info{ .format = ENC_FORMAT_AYUV, .name = "AYUV", .chroma = ENC_CHROMA_444, .bit_depth = 8, .plane_count = 1, .rgb = false },
			format_info{ .format = ENC_FORMAT_Y410, .name = "Y410", .chroma = ENC_CHROMA_444, .bit_depth = 10, .plane_count = 1, .rgb = false },
			format_info{ .format = ENC_FORMAT_Y416, .name = "Y416", .chroma = ENC_CHROMA_444, .bit_depth = 16, .plane_count = 1, .rgb = false },

			format_info{ .format = ENC_FORMAT_RGBA8, .name = "RGBA8", .chroma = ENC_CHROMA_444, .bit_depth = 8, .plane_count = 1, .rgb = true },
			format_info{ .format = ENC_FORMAT_BGRA8, .name = "BGRA8", .chroma = ENC_CHROMA_444, .bit_depth = 8, .plane_count = 1, .rgb = true },
			format_info{ .format = ENC_FORMAT_RGB10A2, .name = "RGB10A2", .chroma = ENC_CHROMA_444, .bit_depth = 10, .plane_count = 1, .rgb = true },
			format_info{ .format = ENC_FORMAT_RGBA16F, .name = "RGBA16F", .chroma = ENC_CHROMA_444, .bit_depth = 16, .plane_count = 1, .rgb = true }
	};

	[[nodiscard]]
	constexpr const format_info* find_format(const enc_format format) noexcept {
		const auto match = std::ranges::find(format_table, format, &format_info::format);

		return match == format_table.end() ? nullptr : &*match;
	}

	[[nodiscard]]
	constexpr enc_bit_depth bit_depth_flag(const std::uint32_t bit_depth) noexcept {
		switch(bit_depth) {
			case 8: return ENC_BIT_DEPTH_8;
			case 10: return ENC_BIT_DEPTH_10;
			case 12: return ENC_BIT_DEPTH_12;
			case 16: return ENC_BIT_DEPTH_16;
			default: return ENC_BIT_DEPTH_NONE;
		}
	}
}

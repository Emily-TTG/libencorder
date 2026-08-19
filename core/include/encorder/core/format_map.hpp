#pragma once

#include <encorder/core/format.hpp>

namespace encorder {
	template<typename native>
	struct format_mapping {
		enc_format format;
		native value;
	};

	// TODO(Emily): Pull out of line into inl.
	template<typename native, std::size_t count>
	class native_format_map {
	private:
		std::array<format_mapping<native>, count> entries;

	public:
		constexpr explicit native_format_map(
				const std::array<format_mapping<native>, count>& source) noexcept :
				entries(source) {}

		[[nodiscard]]
		constexpr std::optional<native> to_native(const enc_format format) const noexcept {
			const auto match = std::ranges::find(entries, format, &format_mapping<native>::format);

			if(match == entries.end()) return std::nullopt;

			return match->value;
		}

		[[nodiscard]]
		constexpr enc_format from_native(const native value) const noexcept {
			const auto match = std::ranges::find(entries, value, &format_mapping<native>::value);

			return match == entries.end() ? ENC_FORMAT_UNDEFINED : match->format;
		}

		[[nodiscard]]
		constexpr std::span<const format_mapping<native>> all() const noexcept {
			return entries;
		}
	};
}

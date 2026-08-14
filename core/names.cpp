#include <encorder/core/error.hpp>

#include <magic_enum/magic_enum_utility.hpp>

template<>
struct magic_enum::customize::enum_range<enc_result> {
	static constexpr int min = ENC_RESULT_MINIMUM;
	static constexpr int max = ENC_RESULT_MAXIMUM;
};

namespace encorder {
	namespace {
		template<typename enum_type>
		constexpr std::size_t enum_description_width = [] {
			std::size_t width = 0;

			for(const auto name : magic_enum::enum_names<enum_type>()) {
				if(name.size() > width) width = name.size();
			}

			return width + 1;
		}();

		// Thanks to https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/.
		template<std::size_t length>
		struct enum_name {
			char value[length];

			// ReSharper disable once CppNonExplicitConvertingConstructor
			constexpr enum_name(const char (&literal)[length]) { // NOLINT(*-pro-type-member-init)
				std::copy_n(literal, length, value);
			}

			// ReSharper disable once CppNonExplicitConversionOperator
			constexpr operator std::array<const char, length>() const {
				return value;
			}

			constexpr char operator[](const std::size_t i) const {
				return value[i];
			}

			// ReSharper disable once CppMemberFunctionMayBeStatic
			[[nodiscard]]
			constexpr std::size_t size() const {
				return length;
			}
		};

		constexpr int to_lower(const char c) {
			// ReSharper disable once CppRedundantParentheses
			return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
		}

		template<typename enum_type, enum_name type_name>
		constexpr auto enum_descriptions = magic_enum::enum_for_each<enum_type>(
				[](const enum_type value) {
					std::array<char, enum_description_width<enum_type>> buffer{};

					const auto name = magic_enum::enum_name<enum_type>(value);

					const std::size_t base = type_name.size();
					for(std::size_t i = base; i < name.size(); ++i) {
						buffer[i - base] = name[i] == '_' ? ' ' : to_lower(name[i]);
					}

					return buffer;
				});

		template<typename enum_type, enum_name type_name>
		constexpr const char* enum_description(
				const enum_type value,
				const char* fallback = "unknown value") noexcept {

			const auto index = magic_enum::enum_index(value);

			return index ? enum_descriptions<enum_type, type_name>[*index].data() : fallback;
		}

#define ENC_ENUM_DESCRIPTION(type, ...) enum_description<type, #type>(__VA_ARGS__)
	}

	ENC_API std::uint32_t enc_version(void) {
		return ENC_VERSION;
	}

	ENC_API const char* enc_result_name(const enc_result value) {
		return ENC_ENUM_DESCRIPTION(enc_result, value, "unrecognised result");
	}

	ENC_API const char* enc_last_error_context(void) {
		const auto detail = last_context();

		return detail.empty() ? "" : detail.data();
	}

	ENC_API const char* enc_backend_name(const enc_backend value) {
		return ENC_ENUM_DESCRIPTION(enc_backend, value, "unrecognised backend");
	}

	ENC_API const char* enc_codec_name(const enc_codec value) {
		return ENC_ENUM_DESCRIPTION(enc_codec, value, "unrecognised codec");
	}

	ENC_API bool enc_codec_has_keyframes(const enc_codec value) {
		return !(value == ENC_CODEC_PRORES || value == ENC_CODEC_MJPEG);
	}
}

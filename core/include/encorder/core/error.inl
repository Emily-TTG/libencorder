#pragma once

#include <encorder/core/error.hpp>

namespace encorder {
	template<typename value>
	enc_result set_error_result(const result<value>& source) {
		if(source) return ENC_RESULT_SUCCESS;

		return set_error_result(source.error());
	}

#define ENC_READ_STRUCT(type, source, version) (read_struct<type>(source, version, #type))

	template<typename structure>
	result<structure> read_struct(
			const void* const source,
			const std::uint32_t version,
			const std::string_view name) {

		const auto checked = check_struct(
				source,
				sizeof(structure),
				version,
				name);

		if(!checked) return std::unexpected(checked.error());

		structure target{};
		std::uint32_t size;
		std::memcpy(&size, source, sizeof(size));
		std::memcpy(&target, source, size);

		return target;
	}

	template<typename... arguments>
	std::unexpected<error> unexpect(
			const enc_result code,
			const std::format_string<arguments...> format,
			arguments&&... values) {

		return std::unexpected(error(
				code,
				std::format(format, std::forward<arguments>(values)...)));
	}
}

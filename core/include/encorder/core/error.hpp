#pragma once

/* TLS holds error description. */

namespace encorder {
	class error {
	private:
		enc_result code;
		std::string context;

	public:
		error() noexcept;
		explicit error(enc_result) noexcept;
		error(enc_result, std::string);

	public:
		[[nodiscard]]
		enc_result get_code() const noexcept;

		[[nodiscard]]
		const std::string& get_context() const noexcept;
	};

	template<typename value>
	using result = std::expected<value, error>;

	[[nodiscard]]
	enc_result set_error_result(const error&);

	[[nodiscard]]
	enc_result set_error_result(enc_result);

	template<typename value>
	[[nodiscard]]
	enc_result set_error_result(const result<value>&);

	void clear_error() noexcept;

	[[nodiscard]]
	std::string_view last_context() noexcept;

	template<typename... arguments>
	[[nodiscard]]
	std::unexpected<error> unexpect(
			enc_result,
			std::format_string<arguments...>,
			arguments&&...);

	[[nodiscard]]
	std::unexpected<error> unexpect(enc_result);

	[[nodiscard]]
	result<void> check_struct(
			const void*,
			std::size_t,
			std::uint32_t,
			std::string_view);

	template<typename structure>
	[[nodiscard]]
	result<structure> read_struct(
			const void*,
			std::uint32_t,
			std::string_view);

// TODO(Emily): Need to add `[[unlikely]]` to all our error paths.
#define ENC_NULL_CHECK(expression) if(!(expression)) [[unlikely]] return unexpect(ENC_RESULT_ERROR_INVALID_ARGUMENT, "`" #expression "` is null");
}

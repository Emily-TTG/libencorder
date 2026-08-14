#include <encorder/core/error.inl>

namespace encorder {
	namespace {
		thread_local std::string context;
	}

	error::error() noexcept :
			code(ENC_RESULT_ERROR_UNKNOWN) {}

	error::error(const enc_result code) noexcept :
			code(code) {}

	error::error(const enc_result code, std::string detail) :
			code(code),
			context(std::move(detail)) {}

	enc_result error::get_code() const noexcept {
		return code;
	}

	const std::string& error::get_context() const noexcept {
		return context;
	}

	enc_result set_error_result(const error& source) {
		context = source.get_context();

		return source.get_code();
	}

	enc_result set_error_result(const enc_result code) {
		context.clear();

		return code;
	}

	void clear_error() noexcept {
		context.clear();
	}

	std::string_view last_context() noexcept {
		return context;
	}

	std::unexpected<error> unexpect(const enc_result code) {
		return std::unexpected(error(code));
	}

	result<void> check_struct(
			const void* const structure,
			const std::size_t expected,
			const std::uint32_t version,
			const std::string_view name) {

		if(!structure) {
			return unexpect(
					ENC_RESULT_ERROR_INVALID_ARGUMENT,
					"{} must not be null",
					name);
		}

		const auto header = static_cast<const enc_struct_header*>(structure);

		if(header->struct_size < sizeof(enc_struct_header) || header->struct_size > expected) {
			// TODO(Emily): Will need "expected `{}..{}`" when/if we have older allowed sizes.
			return unexpect(
					ENC_RESULT_ERROR_INVALID_STRUCT_SIZE,
					"`{}.struct_size` is `{}`, expected `{}`",
					name,
					header->struct_size,
					expected);
		}

		if(header->version > version) {
			return unexpect(
					ENC_RESULT_ERROR_INVALID_STRUCT_VERSION,
					"`{}.version` is `{}`, this build supports up to `{}`",
					name,
					header->version,
					version);
		}

		return {};
	}
}

#pragma once

namespace encorder {
	class logger {
	private:
		enc_log_callback_t sink;
		void* user;
		enc_log_level threshold;

	public:
		logger() noexcept;
		logger(enc_log_callback_t, void*, enc_log_level) noexcept;

		[[nodiscard]]
		bool enabled(enc_log_level) const noexcept;

		void write(enc_log_level, std::string_view) const;

		template<typename... arguments>
		void log(
				enc_log_level,
				std::format_string<arguments...>,
				arguments&&...) const;
	};
}

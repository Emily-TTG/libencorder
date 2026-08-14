#include <encorder/core/logger.hpp>

namespace encorder {
	logger::logger() noexcept :
			sink(nullptr),
			user(nullptr),
			threshold(ENC_LOG_OFF) {}

	logger::logger(
			const enc_log_callback_t sink,
			void* const user,
			const enc_log_level threshold) noexcept :
			sink(sink),
			user(user),
			threshold(threshold) {}

	bool logger::enabled(const enc_log_level level) const noexcept {
		return sink && level >= threshold && threshold != ENC_LOG_OFF;
	}

	void logger::write(
			const enc_log_level level,
			const std::string_view message) const {

		if(!enabled(level)) return;

		const std::string terminated(message);

		sink(level, terminated.c_str(), user);
	}
}

#pragma once

#include <encorder/core/logger.hpp>

namespace encorder {
	template<typename... arguments>
	void logger::log(
			const enc_log_level level,
			const std::format_string<arguments...> format,
			arguments&&... values) const {

		if(!enabled(level)) return;

		write(level, std::format(format, std::forward<arguments>(values)...));
	}
}

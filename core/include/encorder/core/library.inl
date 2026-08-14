#pragma once

namespace encorder {
	template<typename function>
	function shared_library::lookup(const char* const name) const noexcept {
		return reinterpret_cast<function>(symbol(name));
	}
}

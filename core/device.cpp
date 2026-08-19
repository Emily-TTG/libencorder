#include <encorder/core/device.hpp>
#include <encorder/core/error.inl>

namespace encorder {
	device::device() = default;
	device::~device() = default;

	driver::driver() = default;
	driver::~driver() = default;

	result<std::unique_ptr<device>> driver::adopt(const void*) {
		return unexpect(
				ENC_RESULT_ERROR_UNSUPPORTED,
				"backend `{}` cannot adopt an existing device",
				name());
	}
}

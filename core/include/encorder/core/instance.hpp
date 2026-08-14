#pragma once

#include <encorder/core/device.hpp>

namespace encorder {
	struct driver_registration {
		enc_native_kind kind;
		const char* name;
		result<std::unique_ptr<driver>>(*create)(const logger&);
	};

	[[nodiscard]]
	/* List from configure-time codegen. */
	std::span<const driver_registration> registered_drivers() noexcept;
}

struct enc_instance {
	encorder::logger logger;

	enc_backend requested_backends;
	enc_backend available_backends;

	std::vector<std::unique_ptr<encorder::driver>> drivers;

	struct device_slot {
		std::uint32_t owner;
		std::uint32_t local;
	};

	std::vector<device_slot> device_slots;
};

struct enc_device {
	enc_instance* owner;
	std::unique_ptr<encorder::device> implementation;
};

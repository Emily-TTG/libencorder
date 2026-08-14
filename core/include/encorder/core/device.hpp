#pragma once

#include <encorder/core/error.hpp>
#include <encorder/core/logger.hpp>

namespace encorder {
	class device {
	public:
		virtual ~device();

		device(const device&) = delete;
		device& operator=(const device&) = delete;

		device(device&&) = delete;
		device& operator=(device&&) = delete;

	protected:
		device();

	public:
		[[nodiscard]]
		virtual enc_native_kind native_kind() const noexcept = 0;

		[[nodiscard]]
		virtual const enc_device_info& info() const noexcept = 0;

		[[nodiscard]]
		virtual enc_backend backends() const noexcept = 0;
	};

	class driver {
	public:
		virtual ~driver();

		driver(const driver&) = delete;
		driver& operator=(const driver&) = delete;

		driver(driver&&) = delete;
		driver& operator=(driver&&) = delete;

	protected:
		driver();

	public:
		[[nodiscard]]
		virtual enc_native_kind native_kind() const noexcept = 0;

		[[nodiscard]]
		virtual const char* name() const noexcept = 0;

		[[nodiscard]]
		virtual std::span<const enc_device_info> devices() const noexcept = 0;

		[[nodiscard]]
		virtual result<std::unique_ptr<device>> open(std::uint32_t) = 0;
	};
}

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

		[[nodiscard]]
		virtual result<enc_capabilities> query_capabilities(enc_backend, enc_codec) = 0;

		[[nodiscard]]
		virtual result<enc_concurrency_capabilities> query_concurrency(enc_codec) = 0;

		[[nodiscard]]
		virtual result<enc_surface_tier> query_format(enc_backend, enc_codec, enc_format) = 0;

		[[nodiscard]]
		virtual bool encode_capable() const noexcept = 0;
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

		// Adopt a device the host renderer owns via. this native's interop struct.
		[[nodiscard]]
		virtual result<std::unique_ptr<device>> adopt(const void*);
	};
}

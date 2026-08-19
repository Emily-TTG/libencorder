#pragma once

#include <encorder/device/vulkan/capabilities.hpp>

namespace encorder::vulkan {
	class device_base : public encorder::device {
	protected:
		const logger& log;

	private:
		std::mutex cache_mutex;
		// Probing a codec walks the whole chroma/depth matrix.
		std::vector<codec_capabilities> cache;

	protected:
		explicit device_base(const logger&) noexcept;

	protected:
		[[nodiscard]]
		virtual const physical_device_info& physical() const noexcept = 0;

		[[nodiscard]]
		virtual const instance_functions& api() const noexcept = 0;

	public:
		[[nodiscard]]
		enc_native_kind native_kind() const noexcept override;

		[[nodiscard]]
		const enc_device_info& info() const noexcept override;

		[[nodiscard]]
		enc_backend backends() const noexcept override;

		[[nodiscard]]
		result<enc_capabilities> query_capabilities(enc_backend, enc_codec) override;

		[[nodiscard]]
		result<enc_concurrency_capabilities> query_concurrency(enc_codec) override;

		[[nodiscard]]
		result<enc_surface_tier> query_format(enc_backend, enc_codec, enc_format) override;

	public:
		[[nodiscard]]
		result<const codec_capabilities*> codec_details(enc_codec);
	};

	class query_device final : public device_base {
	private:
		const instance_functions& functions;
		const physical_device_info& record;

	public:
		query_device(const logger&, const instance_functions&, const physical_device_info&) noexcept;

	protected:
		[[nodiscard]]
		const physical_device_info& physical() const noexcept override;

		[[nodiscard]]
		const instance_functions& api() const noexcept override;

	public:
		[[nodiscard]]
		bool encode_capable() const noexcept override;
	};

	// Everything encorder borrows from the host renderer.
	struct imported_context {
		VkInstance instance;
		VkPhysicalDevice physical_device;
		VkDevice device;

		std::uint32_t api_version;

		std::uint32_t encode_family;
		std::uint32_t encode_index;

		std::optional<std::uint32_t> compute_family;
		std::optional<std::uint32_t> transfer_family;

		void (*queue_lock)(void*);
		void (*queue_unlock)(void*);
		void* queue_user;

		const VkAllocationCallbacks* allocator;
	};

	// Every vulkan object here belongs to the caller and outlives this object.
	class adopted_device final : public device_base {
	private:
		imported_context context;

		instance_functions functions;
		device_functions device_api;

		physical_device_info record;

	public:
		adopted_device(
				const logger&,
				const imported_context&,
				const instance_functions&,
				const device_functions&,
				const physical_device_info&) noexcept;

	protected:
		[[nodiscard]]
		const physical_device_info& physical() const noexcept override;

		[[nodiscard]]
		const instance_functions& api() const noexcept override;

	public:
		[[nodiscard]]
		bool encode_capable() const noexcept override;

		[[nodiscard]]
		const imported_context& imported() const noexcept;

		[[nodiscard]]
		const device_functions& device_functions_table() const noexcept;

		[[nodiscard]]
		bool can_submit() const noexcept;
	};
}

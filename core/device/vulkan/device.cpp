#include <encorder/device/vulkan/device.hpp>

#include <encorder/core/error.inl>
#include <encorder/core/logger.inl>

using namespace magic_enum::bitwise_operators;

namespace encorder::vulkan {
	device_base::device_base(const logger& source_log) noexcept :
			log(source_log) {}

	enc_native_kind device_base::native_kind() const noexcept {
		return ENC_NATIVE_VULKAN;
	}

	const enc_device_info& device_base::info() const noexcept {
		return physical().info;
	}

	enc_backend device_base::backends() const noexcept {
		return physical().info.backends;
	}

	result<const codec_capabilities*> device_base::codec_details(const enc_codec codec) {
		const std::scoped_lock guard(cache_mutex);

		const auto cached = std::ranges::find_if(
				cache,
				[codec](const codec_capabilities& entry) { return entry.capabilities.codec == codec; });

		if(cached != cache.end()) return &*cached;

		auto queried = query_codec_capabilities(api(), physical(), codec, log);
		if(!queried) return std::unexpected(queried.error());

		cache.push_back(std::move(*queried));

		return &cache.back();
	}

	result<enc_capabilities> device_base::query_capabilities(const enc_backend, const enc_codec codec) {
		const auto details = codec_details(codec);
		if(!details) return std::unexpected(details.error());

		return (*details)->capabilities;
	}

	result<enc_surface_tier> device_base::query_format(
			const enc_backend,
			const enc_codec codec,
			const enc_format format) {

		const auto details = codec_details(codec);
		if(!details) return std::unexpected(details.error());

		return query_format_tier(api(), physical(), **details, format);
	}

	result<enc_concurrency_capabilities> device_base::query_concurrency(const enc_codec codec) {
		const auto& record = physical();

		if(!(record.encode_codecs & codec)) {
			return unexpect(
					ENC_RESULT_ERROR_CODEC_UNSUPPORTED,
					"`{}` does not encode `{}`",
					record.info.name,
					enc_codec_name(codec));
		}

		enc_concurrency_capabilities concurrency{};

		concurrency.struct_size = sizeof(enc_concurrency_capabilities);
		concurrency.version = 0;

		/*
		 * Vulkan exposes no session limit. On NVIDIA a video session does consume
		 * an NVENC slot, but the limit is transparent.
		 */
		concurrency.max_sessions = 0;
		concurrency.source = ENC_SESSION_LIMIT_UNKNOWN;

		concurrency.active_sessions = 0;

		const auto family = std::ranges::find_if(
				record.families,
				[&](const queue_family& entry) {
					return record.encode_family && entry.index == *record.encode_family;
				});

		concurrency.encode_engine_count = family == record.families.end() ? 0 : family->queue_count;
		concurrency.sessions_share_engine = concurrency.encode_engine_count <= 1;

		return concurrency;
	}

	query_device::query_device(
			const logger& source_log,
			const instance_functions& source_functions,
			const physical_device_info& source_record) noexcept :

			device_base(source_log),
			functions(source_functions),
			record(source_record) {}

	const physical_device_info& query_device::physical() const noexcept {
		return record;
	}

	const instance_functions& query_device::api() const noexcept {
		return functions;
	}

	bool query_device::encode_capable() const noexcept {
		return false;
	}

	adopted_device::adopted_device(
			const logger& source_log,
			const imported_context& source_context,
			const instance_functions& source_functions,
			const device_functions& source_device_api,
			const physical_device_info& source_record) noexcept :

			device_base(source_log),
			context(source_context),
			functions(source_functions),
			device_api(source_device_api),
			record(source_record) {}

	const physical_device_info& adopted_device::physical() const noexcept {
		return record;
	}

	const instance_functions& adopted_device::api() const noexcept {
		return functions;
	}

	bool adopted_device::encode_capable() const noexcept {
		return true;
	}

	const imported_context& adopted_device::imported() const noexcept {
		return context;
	}

	const device_functions& adopted_device::device_functions_table() const noexcept {
		return device_api;
	}

	bool adopted_device::can_submit() const noexcept {
		return context.queue_lock && context.queue_unlock;
	}
}

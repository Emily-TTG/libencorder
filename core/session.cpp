#include <encorder/core/session.hpp>

namespace encorder {
	namespace {
		[[nodiscard]]
		std::optional<std::size_t> slot_of(const enc_codec codec) noexcept {
			const auto value = magic_enum::enum_underlying(codec);
			if(std::popcount(value) != 1) return std::nullopt;

			if(value >= session_registry::slot_count) return std::nullopt;

			return static_cast<std::size_t>(std::countr_zero(value));
		}
	}

	session_registry::session_registry() noexcept : counts{} {}

	std::uint32_t session_registry::active(const enc_codec codec) const noexcept {
		const auto slot = slot_of(codec);

		if(!slot) {
			/* A mask asks "how many across these codecs?". */
			std::uint32_t total = 0;

			for(const auto& count : counts) {
				total += count.load(std::memory_order_relaxed);
			}

			return total;
		}

		return counts[*slot].load(std::memory_order_relaxed);
	}

	void session_registry::acquire(const enc_codec codec) noexcept {
		if(const auto slot = slot_of(codec)) {
			counts[*slot].fetch_add(1, std::memory_order_relaxed);
		}
	}

	void session_registry::release(const enc_codec codec) noexcept {
		if(const auto slot = slot_of(codec)) {
			counts[*slot].fetch_sub(1, std::memory_order_relaxed);
		}
	}
}

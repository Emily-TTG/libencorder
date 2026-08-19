#pragma once

namespace encorder {
	class session_registry {
	public:
		/* Indexed by the bit position of a single-bit `enc_codec`. */
		static constexpr std::size_t slot_count = 8;

	private:
		std::array<std::atomic<std::uint32_t>, slot_count> counts;

	public:
		session_registry() noexcept;

		session_registry(const session_registry&) = delete;
		session_registry& operator=(const session_registry&) = delete;

		session_registry(session_registry&&) = delete;
		session_registry& operator=(session_registry&&) = delete;

	public:
		[[nodiscard]]
		std::uint32_t active(enc_codec) const noexcept;

		void acquire(enc_codec) noexcept;
		void release(enc_codec) noexcept;
	};
}

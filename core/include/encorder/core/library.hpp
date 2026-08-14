#pragma once

namespace encorder {
	class shared_library {
	private:
		void* handle;

	public:
		shared_library() noexcept;
		~shared_library();

		shared_library(const shared_library&) = delete;
		shared_library& operator=(const shared_library&) = delete;

		shared_library(shared_library&&) noexcept;
		shared_library& operator=(shared_library&&) noexcept;

		[[nodiscard]]
		bool open(std::span<const char* const>) noexcept;

		void close() noexcept;

		[[nodiscard]]
		bool valid() const noexcept;

		[[nodiscard]]
		void* symbol(const char*) const noexcept;

		template<typename function>
		[[nodiscard]]
		function lookup(const char*) const noexcept;
	};
}

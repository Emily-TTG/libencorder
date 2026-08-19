#include <encorder/core/library.hpp>
#include <encorder/core/error.inl>

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
# define NOMINMAX
#endif

#include <windows.h>

#include <system_error>

namespace encorder {
	shared_library::shared_library() noexcept :
			handle(nullptr) {}

	shared_library::~shared_library() {
		close();
	}

	shared_library::shared_library(shared_library&& other) noexcept :
			handle(std::exchange(other.handle, nullptr)) {}

	shared_library& shared_library::operator=(shared_library&& other) noexcept {
		if(this != &other) {
			close();

			handle = std::exchange(other.handle, nullptr);
		}

		return *this;
	}

	result<void> shared_library::open(const std::span<const char* const> candidates) {
		close();

		std::string failures;

		if(candidates.empty()) {
			return unexpect(
					ENC_RESULT_ERROR_INVALID_ARGUMENT,
					"no library candidates were supplied");
		}

		for(const auto* const candidate : candidates) {
			handle = LoadLibraryA(candidate);

			if(handle) return {};

			const auto reason = static_cast<int>(GetLastError());

			if(!failures.empty()) failures += "; ";

			failures += std::format(
					"`{}`: {}",
					candidate,
					std::system_category().message(reason));
		}

		return unexpect(ENC_RESULT_ERROR_INITIALIZATION_FAILED, "{}", failures);
	}

	void shared_library::close() noexcept {
		if(!handle) return;

		FreeLibrary(static_cast<HMODULE>(handle));

		handle = nullptr;
	}

	bool shared_library::valid() const noexcept {
		return handle != nullptr;
	}

	void* shared_library::symbol(const char* const name) const noexcept {
		if(!handle) return nullptr;

		return reinterpret_cast<void*>(
				GetProcAddress(static_cast<HMODULE>(handle), name));
	}
}

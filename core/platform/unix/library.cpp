#include <encorder/core/library.hpp>

#include <dlfcn.h>

namespace encorder {
	// TODO(Emily): Elegant way to get dlerr out of here?
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

	bool shared_library::open(const std::span<const char* const> candidates) noexcept {
		close();

		for(const auto* const candidate : candidates) {
			handle = dlopen(candidate, RTLD_LAZY | RTLD_LOCAL);

			if(handle) return true;
		}

		return false;
	}

	void shared_library::close() noexcept {
		if(!handle) return;

		dlclose(handle);

		handle = nullptr;
	}

	bool shared_library::valid() const noexcept {
		return handle != nullptr;
	}

	void* shared_library::symbol(const char* const name) const noexcept {
		return handle ? dlsym(handle, name) : nullptr;
	}
}

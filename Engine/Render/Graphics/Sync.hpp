#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"

class Semaphore {
private:
	vk::Semaphore semaphore{};

	Context* context{};

public:
	Semaphore(Context& context);
	~Semaphore();

	Semaphore(const Semaphore&) = delete;
	Semaphore& operator=(const Semaphore&) = delete;

	Semaphore(Semaphore&& other) noexcept;
	Semaphore& operator=(Semaphore&& other) noexcept;

	vk::Semaphore get() const&;
	vk::Semaphore get() const&& = delete;
};

class Fence {
private:
	vk::Fence fence{};

	Context* context{};

public:
	Fence(Context& context, bool signaled = true);

	Fence(const Fence&) = delete;
	Fence& operator=(const Fence&) = delete;

	Fence(Fence&& other) noexcept;
	Fence& operator=(Fence&& other) noexcept;

	~Fence();

	void wait(uint64_t timeout = std::numeric_limits<uint64_t>::max()) const;
	void reset();

	bool signaled() const;

	vk::Fence get() const&;
	vk::Fence get() const&& = delete;
};

#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"

class VulkanSemaphore {
private:
	vk::Semaphore semaphore{};

	VulkanContext* context{};

public:
	VulkanSemaphore(VulkanContext& context);
	~VulkanSemaphore();

	VulkanSemaphore(const VulkanSemaphore&) = delete;
	VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;

	VulkanSemaphore(VulkanSemaphore&& other) noexcept;
	VulkanSemaphore& operator=(VulkanSemaphore&& other) noexcept;

	vk::Semaphore get() const&;
	vk::Semaphore get() const&& = delete;
};

class VulkanFence {
private:
	vk::Fence fence{};

	VulkanContext* context{};

public:
	VulkanFence(VulkanContext& context, bool signaled = true);

	VulkanFence(const VulkanFence&) = delete;
	VulkanFence& operator=(const VulkanFence&) = delete;

	VulkanFence(VulkanFence&& other) noexcept;
	VulkanFence& operator=(VulkanFence&& other) noexcept;

	~VulkanFence();

	void wait(uint64_t timeout = std::numeric_limits<uint64_t>::max()) const;
	void reset();

	bool signaled() const;

	vk::Fence get() const&;
	vk::Fence get() const&& = delete;
};

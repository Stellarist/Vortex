#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"

class VulkanCommandBuffer {
private:
	vk::CommandBuffer command{};

public:
	VulkanCommandBuffer() = default;
	VulkanCommandBuffer(vk::CommandBuffer command);
	~VulkanCommandBuffer() = default;

	void begin(vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	void end();

	vk::CommandBuffer get() const&;
	vk::CommandBuffer get() const&& = delete;

	bool operator==(const VulkanCommandBuffer& other) const = default;
};

class VulkanCommandPool {
private:
	vk::CommandPool            pool{};
	std::vector<VulkanCommandBuffer> buffers;

	VulkanContext* context{};

public:
	VulkanCommandPool(VulkanContext&           context,
	    uint32_t                   queue_family_index,
	    vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	~VulkanCommandPool();

	VulkanCommandPool(const VulkanCommandPool&) = delete;
	VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;

	VulkanCommandPool(VulkanCommandPool&& other) noexcept;
	VulkanCommandPool& operator=(VulkanCommandPool&& other) noexcept;

	auto allocate(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) -> VulkanCommandBuffer;
	auto allocate(uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) -> std::vector<VulkanCommandBuffer>;

	void free(VulkanCommandBuffer buffer);
	void free(std::span<const VulkanCommandBuffer> buffers);

	void reset(vk::CommandPoolResetFlags flags = {});

	vk::CommandPool get() const&;
	vk::CommandPool get() const&& = delete;
};

#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"

void begin(vk::CommandBuffer command);
void end(vk::CommandBuffer command);

#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"

class CommandPool {
private:
	vk::CommandPool                pool{};
	std::vector<vk::CommandBuffer> buffers;

	Context* context{};

public:
	CommandPool(Context&                   context,
	            uint32_t                   queue_family_index,
	            vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	CommandPool(const CommandPool&) = delete;
	CommandPool& operator=(const CommandPool&) = delete;

	CommandPool(CommandPool&& other) noexcept;
	CommandPool& operator=(CommandPool&& other) noexcept;

	~CommandPool();

	vk::CommandBuffer              allocate(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);
	std::vector<vk::CommandBuffer> allocate(uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

	void free(vk::CommandBuffer buffer);
	void free(std::span<const vk::CommandBuffer> buffers);

	void reset(vk::CommandPoolResetFlags flags = {});

	vk::CommandPool get() const&;
	vk::CommandPool get() const&& = delete;

	static void begin(vk::CommandBuffer command, vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	static void end(vk::CommandBuffer command);
};

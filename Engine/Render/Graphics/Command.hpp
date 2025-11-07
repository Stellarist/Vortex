#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"

class CommandBuffer {
private:
	vk::CommandBuffer command{};

public:
	CommandBuffer() = default;
	CommandBuffer(vk::CommandBuffer command);
	~CommandBuffer() = default;

	void begin(vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	void end();

	vk::CommandBuffer get() const&;
	vk::CommandBuffer get() const&& = delete;

	bool operator==(const CommandBuffer& other) const = default;
};

class CommandPool {
private:
	vk::CommandPool            pool{};
	std::vector<CommandBuffer> buffers;

	Context* context{};

public:
	CommandPool(Context&           context,
	    uint32_t                   queue_family_index,
	    vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	~CommandPool();

	CommandPool(const CommandPool&) = delete;
	CommandPool& operator=(const CommandPool&) = delete;

	CommandPool(CommandPool&& other) noexcept;
	CommandPool& operator=(CommandPool&& other) noexcept;

	auto allocate(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) -> CommandBuffer;
	auto allocate(uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) -> std::vector<CommandBuffer>;

	void free(CommandBuffer buffer);
	void free(std::span<const CommandBuffer> buffers);

	void reset(vk::CommandPoolResetFlags flags = {});

	vk::CommandPool get() const&;
	vk::CommandPool get() const&& = delete;
};

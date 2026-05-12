#include "VulkanCommand.hpp"

#include "VulkanDevice.hpp"

VulkanCommandBuffer::VulkanCommandBuffer(vk::CommandBuffer command) :
    command(command)
{}

void VulkanCommandBuffer::begin(vk::CommandBufferUsageFlags flags)
{
	if (!command)
		throw std::runtime_error("Invalid command buffer");

	vk::CommandBufferBeginInfo begin_info{};
	begin_info.setFlags(flags);

	command.begin(begin_info);
}

void VulkanCommandBuffer::end()
{
	if (!command)
		throw std::runtime_error("Invalid command buffer");

	command.end();
}

vk::CommandBuffer VulkanCommandBuffer::get() const&
{
	return command;
}

VulkanCommandPool::VulkanCommandPool(VulkanContext& context, uint32_t queue_family_index,
    vk::CommandPoolCreateFlags flags) : context(&context)
{
	vk::CommandPoolCreateInfo create_info{};
	create_info.setFlags(flags).setQueueFamilyIndex(queue_family_index);

	pool = context.getDevice().logical().createCommandPool(create_info);
}

VulkanCommandPool::~VulkanCommandPool()
{
	if (context && pool)
		context->getDevice().logical().destroyCommandPool(pool);
}

VulkanCommandPool::VulkanCommandPool(VulkanCommandPool&& other) noexcept
    : pool(std::exchange(other.pool, nullptr)),
      buffers(std::move(other.buffers)),
      context(std::exchange(other.context, nullptr)) {}

VulkanCommandPool& VulkanCommandPool::operator=(VulkanCommandPool&& other) noexcept
{
	if (this != &other) {
		if (context && pool)
			context->getDevice().logical().destroyCommandPool(pool);

		pool = std::exchange(other.pool, nullptr);
		buffers = std::move(other.buffers);
		context = std::exchange(other.context, nullptr);
	}

	return *this;
}

VulkanCommandBuffer VulkanCommandPool::allocate(vk::CommandBufferLevel level)
{
	return allocate(1, level).front();
}

std::vector<VulkanCommandBuffer> VulkanCommandPool::allocate(uint32_t count, vk::CommandBufferLevel level)
{
	if (!pool)
		throw std::runtime_error("Invalid command pool");

	vk::CommandBufferAllocateInfo alloc_info{};
	alloc_info.setCommandPool(pool)
	    .setLevel(level)
	    .setCommandBufferCount(count);
	auto vk_cmds = context->getDevice().logical().allocateCommandBuffers(alloc_info);

	std::vector<VulkanCommandBuffer> new_cmds;
	new_cmds.reserve(vk_cmds.size());
	for (const auto& vk_cmd : vk_cmds) {
		buffers.emplace_back(vk_cmd);
		new_cmds.emplace_back(vk_cmd);
	}

	return new_cmds;
}

void VulkanCommandPool::free(VulkanCommandBuffer buffer)
{
	free(std::span{&buffer, 1});
}

void VulkanCommandPool::free(std::span<const VulkanCommandBuffer> to_free)
{
	if (!pool || to_free.empty())
		return;

	std::vector<vk::CommandBuffer> vk_cmds;
	vk_cmds.reserve(to_free.size());
	for (const auto& cmd : to_free)
		vk_cmds.push_back(cmd.get());
	context->getDevice().logical().freeCommandBuffers(pool, vk_cmds);

	std::erase_if(buffers, [&](const VulkanCommandBuffer& c) {
		return std::find(to_free.begin(), to_free.end(), c) != to_free.end();
	});
}

void VulkanCommandPool::reset(vk::CommandPoolResetFlags flags)
{
	if (pool) {
		context->getDevice().logical().resetCommandPool(pool, flags);
		buffers.clear();
	}
}

vk::CommandPool VulkanCommandPool::get() const&
{
	return pool;
}

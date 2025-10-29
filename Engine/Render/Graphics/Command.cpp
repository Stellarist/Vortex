#include "Command.hpp"

CommandPool::CommandPool(Context&                   context,
                         uint32_t                   queue_family_index,
                         vk::CommandPoolCreateFlags flags) :
    context(&context)
{
	vk::CommandPoolCreateInfo create_info{};
	create_info.setFlags(flags)
	    .setQueueFamilyIndex(queue_family_index);

	pool = context.getLogicalDevice().createCommandPool(create_info);
}

CommandPool::CommandPool(CommandPool&& other) noexcept :
    pool(std::exchange(other.pool, nullptr)),
    buffers(std::move(other.buffers)),
    context(std::exchange(other.context, nullptr))
{}

CommandPool& CommandPool::operator=(CommandPool&& other) noexcept
{
	if (this != &other) {
		if (context && pool)
			context->getLogicalDevice().destroyCommandPool(pool);

		pool = std::exchange(other.pool, nullptr);
		buffers = std::move(other.buffers);
		context = std::exchange(other.context, nullptr);
	}

	return *this;
}

CommandPool::~CommandPool()
{
	if (context && pool)
		context->getLogicalDevice().destroyCommandPool(pool);
}

vk::CommandBuffer CommandPool::allocate(vk::CommandBufferLevel level)
{
	return allocate(1, level).front();
}

std::vector<vk::CommandBuffer> CommandPool::allocate(uint32_t count, vk::CommandBufferLevel level)
{
	if (!pool)
		throw std::runtime_error("Invalid command pool");

	vk::CommandBufferAllocateInfo alloc_info{};
	alloc_info.setCommandPool(pool)
	    .setLevel(level)
	    .setCommandBufferCount(count);

	auto new_buffers = context->getLogicalDevice().allocateCommandBuffers(alloc_info);
	buffers.insert(buffers.end(), new_buffers.begin(), new_buffers.end());

	return new_buffers;
}

void CommandPool::free(vk::CommandBuffer buffer)
{
	free(std::span<const vk::CommandBuffer>{&buffer, 1});
}

void CommandPool::free(std::span<const vk::CommandBuffer> to_free)
{
	if (!pool || to_free.empty())
		return;

	context->getLogicalDevice().freeCommandBuffers(pool, to_free);
	std::erase_if(buffers, [&](const vk::CommandBuffer& buf) {
		return std::find(to_free.begin(), to_free.end(), buf) != to_free.end();
	});
}

void CommandPool::reset(vk::CommandPoolResetFlags flags)
{
	if (!pool)
		throw std::runtime_error("Invalid command pool");

	context->getLogicalDevice().resetCommandPool(pool, flags);
}

vk::CommandPool CommandPool::get() const&
{
	return pool;
}

void CommandPool::begin(vk::CommandBuffer command, vk::CommandBufferUsageFlags flags)
{
	if (!command)
		return;

	vk::CommandBufferBeginInfo begin_info{};
	begin_info.setFlags(flags);

	command.begin(begin_info);
}

void CommandPool::end(vk::CommandBuffer command)
{
	if (!command)
		return;

	command.end();
}

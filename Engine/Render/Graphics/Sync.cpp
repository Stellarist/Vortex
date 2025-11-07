#include "Sync.hpp"

#include "Device.hpp"

#include <vulkan/vulkan.h>

Semaphore::Semaphore(Context& context) :
    context(&context)
{
	vk::SemaphoreCreateInfo create_info{};
	semaphore = context.getDevice().logical().createSemaphore(create_info);
}

Semaphore::~Semaphore()
{
	if (context && semaphore)
		context->getDevice().logical().destroySemaphore(semaphore);
}

Semaphore::Semaphore(Semaphore&& other) noexcept :
    semaphore(std::exchange(other.semaphore, nullptr)),
    context(std::exchange(other.context, nullptr))
{}

Semaphore& Semaphore::operator=(Semaphore&& other) noexcept
{
	if (this != &other) {
		if (context && semaphore)
			context->getDevice().logical().destroySemaphore(semaphore);

		semaphore = std::exchange(other.semaphore, nullptr);
		context = std::exchange(other.context, nullptr);
	}

	return *this;
}

vk::Semaphore Semaphore::get() const&
{
	return semaphore;
}

Fence::Fence(Context& context, bool signaled) :
    context(&context)
{
	vk::FenceCreateInfo create_info{};
	if (signaled)
		create_info.setFlags(vk::FenceCreateFlagBits::eSignaled);

	fence = context.getDevice().logical().createFence(create_info);
}

Fence::~Fence()
{
	if (context && fence)
		context->getDevice().logical().destroyFence(fence);
}

Fence::Fence(Fence&& other) noexcept :
    fence(std::exchange(other.fence, nullptr)),
    context(std::exchange(other.context, nullptr))
{}

Fence& Fence::operator=(Fence&& other) noexcept
{
	if (this != &other) {
		if (context && fence)
			context->getDevice().logical().destroyFence(fence);

		fence = std::exchange(other.fence, nullptr);
		context = std::exchange(other.context, nullptr);
	}

	return *this;
}

void Fence::wait(uint64_t timeout) const
{
	if (!fence)
		throw std::runtime_error("Invalid fence");

	auto result = context->getDevice().logical().waitForFences(fence, vk::True, timeout);
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("Failed to wait for fence");
}

void Fence::reset()
{
	if (!fence)
		throw std::runtime_error("Invalid fence");

	context->getDevice().logical().resetFences(fence);
}

bool Fence::signaled() const
{
	if (!fence)
		return false;

	return context->getDevice().logical().getFenceStatus(fence) == vk::Result::eSuccess;
}

vk::Fence Fence::get() const&
{
	return fence;
}

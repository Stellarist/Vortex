#include "VulkanSync.hpp"

#include "VulkanDevice.hpp"

#include <vulkan/vulkan.h>

VulkanSemaphore::VulkanSemaphore(VulkanContext& context) :
    context(&context)
{
	vk::SemaphoreCreateInfo create_info{};
	semaphore = context.getDevice().logical().createSemaphore(create_info);
}

VulkanSemaphore::~VulkanSemaphore()
{
	if (context && semaphore)
		context->getDevice().logical().destroySemaphore(semaphore);
}

VulkanSemaphore::VulkanSemaphore(VulkanSemaphore&& other) noexcept :
    semaphore(std::exchange(other.semaphore, nullptr)),
    context(std::exchange(other.context, nullptr))
{}

VulkanSemaphore& VulkanSemaphore::operator=(VulkanSemaphore&& other) noexcept
{
	if (this != &other) {
		if (context && semaphore)
			context->getDevice().logical().destroySemaphore(semaphore);

		semaphore = std::exchange(other.semaphore, nullptr);
		context = std::exchange(other.context, nullptr);
	}

	return *this;
}

vk::Semaphore VulkanSemaphore::get() const&
{
	return semaphore;
}

VulkanFence::VulkanFence(VulkanContext& context, bool signaled) :
    context(&context)
{
	vk::FenceCreateInfo create_info{};
	if (signaled)
		create_info.setFlags(vk::FenceCreateFlagBits::eSignaled);

	fence = context.getDevice().logical().createFence(create_info);
}

VulkanFence::~VulkanFence()
{
	if (context && fence)
		context->getDevice().logical().destroyFence(fence);
}

VulkanFence::VulkanFence(VulkanFence&& other) noexcept :
    fence(std::exchange(other.fence, nullptr)),
    context(std::exchange(other.context, nullptr))
{}

VulkanFence& VulkanFence::operator=(VulkanFence&& other) noexcept
{
	if (this != &other) {
		if (context && fence)
			context->getDevice().logical().destroyFence(fence);

		fence = std::exchange(other.fence, nullptr);
		context = std::exchange(other.context, nullptr);
	}

	return *this;
}

void VulkanFence::wait(uint64_t timeout) const
{
	if (!fence)
		throw std::runtime_error("Invalid fence");

	auto result = context->getDevice().logical().waitForFences(fence, vk::True, timeout);
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("Failed to wait for fence");
}

void VulkanFence::reset()
{
	if (!fence)
		throw std::runtime_error("Invalid fence");

	context->getDevice().logical().resetFences(fence);
}

bool VulkanFence::signaled() const
{
	if (!fence)
		return false;

	return context->getDevice().logical().getFenceStatus(fence) == vk::Result::eSuccess;
}

vk::Fence VulkanFence::get() const&
{
	return fence;
}

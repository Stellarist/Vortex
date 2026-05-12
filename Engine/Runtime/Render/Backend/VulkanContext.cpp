#include "VulkanContext.hpp"

#include <ranges>

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanCommand.hpp"
#include "VulkanSync.hpp"

VulkanContext::VulkanContext(Window& window) :
    window(&window)
{
	requestExtensions();
	requestLayers();

	createInstance();
	createSurface();
	createDevice();
	createSwapChain();
	createCommandPools();
}

VulkanContext::~VulkanContext()
{
	transfer_command_pool.reset();
	graphics_command_pool.reset();
	swap_chain.reset();
	device.reset();
	instance.destroySurfaceKHR(surface);
	instance.destroy();
}

void VulkanContext::createInstance()
{
	auto layers = requestLayers();
	auto extensions = requestExtensions();

	vk::ApplicationInfo app_info{};
	app_info.setApiVersion(VK_API_VERSION_1_4);

	vk::InstanceCreateInfo create_info{};
	create_info.setPApplicationInfo(&app_info)
	    .setEnabledLayerCount(layers.size())
	    .setPEnabledLayerNames(layers)
	    .setEnabledExtensionCount(extensions.size())
	    .setPEnabledExtensionNames(extensions);

	instance = vk::createInstance(create_info);
}

void VulkanContext::createSurface()
{
	VkSurfaceKHR csurface{};
	if (!SDL_Vulkan_CreateSurface(window->get(), instance, nullptr, &csurface))
		throw std::runtime_error(SDL_GetError());

	surface = std::move(csurface);
}

void VulkanContext::createDevice()
{
	device = std::make_unique<VulkanDevice>(*this);
}

void VulkanContext::createSwapChain()
{
	swap_chain = std::make_unique<VulkanSwapChain>(*window, *this);
}

void VulkanContext::createCommandPools()
{
	graphics_command_pool = std::make_unique<VulkanCommandPool>(
	    *this,
	    device->graphicsQueueIndex(),
	    vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	transfer_command_pool = std::make_unique<VulkanCommandPool>(
	    *this,
	    device->graphicsQueueIndex(),
	    vk::CommandPoolCreateFlagBits::eTransient);
}

void VulkanContext::execute(std::function<void(VulkanCommandBuffer)> func)
{
	auto command = transfer_command_pool->allocate();

	command.begin();
	func(command);
	command.end();

	VulkanFence fence(*this, false);
	submit({command}, &fence);
	fence.wait();

	transfer_command_pool->free(command);
}

void VulkanContext::submit(const std::vector<VulkanCommandBuffer>& cmds, VulkanFence* fence,
    const std::vector<VulkanSemaphore*>&             waits,
    const std::vector<VulkanSemaphore*>&             signals,
    const std::vector<vk::PipelineStageFlags>& stages)
{
	auto vk_cmds = cmds | std::views::transform([](const VulkanCommandBuffer& cmd) { return cmd.get(); }) | std::ranges::to<std::vector>();
	auto vk_waits = waits | std::views::transform([](VulkanSemaphore* s) { return s->get(); }) | std::ranges::to<std::vector>();
	auto vk_signals = signals | std::views::transform([](VulkanSemaphore* s) { return s->get(); }) | std::ranges::to<std::vector>();

	vk::SubmitInfo submit_info;
	submit_info.setCommandBuffers(vk_cmds)
	    .setWaitSemaphores(vk_waits)
	    .setSignalSemaphores(vk_signals)
	    .setWaitDstStageMask(stages);

	device->graphicsQueue().submit(submit_info, fence ? fence->get() : nullptr);
}

void VulkanContext::present(const std::vector<uint32_t>& images,
    const std::vector<VulkanSemaphore*>&                 waits)
{
	auto vk_sc = swap_chain->get();
	auto vk_waits = waits
	    | std::views::transform([](VulkanSemaphore* s) { return s->get(); })
	    | std::ranges::to<std::vector>();

	vk::PresentInfoKHR present_info;
	present_info.setImageIndices(images)
	    .setSwapchains(vk_sc)
	    .setWaitSemaphores(vk_waits);

	if (device->presentQueue().presentKHR(present_info) != vk::Result::eSuccess)
		throw std::runtime_error("Failed to present swap chain image");
}

std::vector<const char*> VulkanContext::requestExtensions()
{
	auto count = 0u;
	auto sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&count);

	std::vector<const char*> extensions;
	for (uint32_t i = 0; i < count; i++)
		extensions.push_back(sdl_extensions[i]);

	return extensions;
}

std::vector<const char*> VulkanContext::requestLayers()
{
	std::vector<const char*> layers = {
	    "VK_LAYER_KHRONOS_validation",
	};

	return layers;
}

vk::Instance VulkanContext::getInstance() const
{
	return instance;
}

vk::SurfaceKHR VulkanContext::getSurface() const
{
	return surface;
}

VulkanDevice& VulkanContext::getDevice() const
{
	return *device;
}

VulkanSwapChain& VulkanContext::getSwapChain() const
{
	return *swap_chain;
}

VulkanCommandPool& VulkanContext::getGraphicsCommandPool() const
{
	return *graphics_command_pool;
}

VulkanCommandPool& VulkanContext::getTransferCommandPool() const
{
	return *transfer_command_pool;
}

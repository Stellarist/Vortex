#include "Context.hpp"

#include <ranges>

#include "Device.hpp"
#include "SwapChain.hpp"
#include "Command.hpp"
#include "Sync.hpp"

Context::Context(Window& window) :
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

Context::~Context()
{
	transfer_command_pool.reset();
	graphics_command_pool.reset();
	swap_chain.reset();
	device.reset();
	instance.destroySurfaceKHR(surface);
	instance.destroy();
}

void Context::createInstance()
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

void Context::createSurface()
{
	VkSurfaceKHR csurface{};
	if (!SDL_Vulkan_CreateSurface(window->get(), instance, nullptr, &csurface))
		throw std::runtime_error(SDL_GetError());

	surface = std::move(csurface);
}

void Context::createDevice()
{
	device = std::make_unique<Device>(*this);
}

void Context::createSwapChain()
{
	swap_chain = std::make_unique<SwapChain>(*window, *this);
}

void Context::createCommandPools()
{
	graphics_command_pool = std::make_unique<CommandPool>(
	    *this,
	    device->graphicsQueueIndex(),
	    vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	transfer_command_pool = std::make_unique<CommandPool>(
	    *this,
	    device->graphicsQueueIndex(),
	    vk::CommandPoolCreateFlagBits::eTransient);
}

void Context::execute(std::function<void(CommandBuffer)> func)
{
	auto command = transfer_command_pool->allocate();

	command.begin();
	func(command);
	command.end();

	Fence fence(*this, false);
	submit({command}, &fence);
	fence.wait();

	transfer_command_pool->free(command);
}

void Context::submit(const std::vector<CommandBuffer>& cmds, Fence* fence,
    const std::vector<Semaphore*>&             waits,
    const std::vector<Semaphore*>&             signals,
    const std::vector<vk::PipelineStageFlags>& stages)
{
	auto vk_cmds = cmds | std::views::transform([](const CommandBuffer& cmd) { return cmd.get(); }) | std::ranges::to<std::vector>();
	auto vk_waits = waits | std::views::transform([](Semaphore* s) { return s->get(); }) | std::ranges::to<std::vector>();
	auto vk_signals = signals | std::views::transform([](Semaphore* s) { return s->get(); }) | std::ranges::to<std::vector>();

	vk::SubmitInfo submit_info;
	submit_info.setCommandBuffers(vk_cmds)
	    .setWaitSemaphores(vk_waits)
	    .setSignalSemaphores(vk_signals)
	    .setWaitDstStageMask(stages);

	device->graphicsQueue().submit(submit_info, fence ? fence->get() : nullptr);
}

void Context::present(const std::vector<uint32_t>& images,
    const std::vector<Semaphore*>&                 waits)
{
	auto vk_sc = swap_chain->get();
	auto vk_waits = waits
	    | std::views::transform([](Semaphore* s) { return s->get(); })
	    | std::ranges::to<std::vector>();

	vk::PresentInfoKHR present_info;
	present_info.setImageIndices(images)
	    .setSwapchains(vk_sc)
	    .setWaitSemaphores(vk_waits);

	if (device->presentQueue().presentKHR(present_info) != vk::Result::eSuccess)
		throw std::runtime_error("Failed to present swap chain image");
}

std::vector<const char*> Context::requestExtensions()
{
	auto count = 0u;
	auto sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&count);

	std::vector<const char*> extensions;
	for (uint32_t i = 0; i < count; i++)
		extensions.push_back(sdl_extensions[i]);

	return extensions;
}

std::vector<const char*> Context::requestLayers()
{
	std::vector<const char*> layers = {
	    "VK_LAYER_KHRONOS_validation",
	};

	return layers;
}

vk::Instance Context::getInstance() const
{
	return instance;
}

vk::SurfaceKHR Context::getSurface() const
{
	return surface;
}

Device& Context::getDevice() const
{
	return *device;
}

SwapChain& Context::getSwapChain() const
{
	return *swap_chain;
}

CommandPool& Context::getGraphicsCommandPool() const
{
	return *graphics_command_pool;
}

CommandPool& Context::getTransferCommandPool() const
{
	return *transfer_command_pool;
}

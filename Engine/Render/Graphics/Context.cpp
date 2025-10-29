#include "Context.hpp"

#include <set>
#include <print>

#include "Command.hpp"
#include "Sync.hpp"

Context::Context(Window& window) :
    window(&window)
{
	createInstance();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createCommandPools();
}

Context::~Context()
{
	transfer_command_pool.reset();
	graphics_command_pool.reset();
	logical_device.destroy();
	instance.destroySurfaceKHR(surface);
	instance.destroy();
}

void Context::createInstance()
{
	uint32_t   count = 0;
	std::array layers = {"VK_LAYER_KHRONOS_validation"};

	auto extensions = SDL_Vulkan_GetInstanceExtensions(&count);

	vk::ApplicationInfo app_info{};
	app_info.setApiVersion(VK_API_VERSION_1_4);

	vk::InstanceCreateInfo create_info{};
	create_info.setPApplicationInfo(&app_info)
	    .setEnabledLayerCount(layers.size())
	    .setPEnabledLayerNames(layers)
	    .setEnabledExtensionCount(count)
	    .setPpEnabledExtensionNames(extensions);

	instance = vk::createInstance(create_info);
}

void Context::createSurface()
{
	VkSurfaceKHR csurface{};
	if (!SDL_Vulkan_CreateSurface(window->get(), instance, nullptr, &csurface)) {
		std::println("Failed to create Vulkan surface: {}", SDL_GetError());
		throw std::runtime_error("Vulkan surface creation failed");
	}

	surface = std::move(csurface);
}

void Context::pickPhysicalDevice()
{
	auto devices = instance.enumeratePhysicalDevices();
	for (const auto& context : devices) {
		vk::PhysicalDeviceProperties properties = context.getProperties();
		if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			physical_device = context;
			return;
		}
	}

	physical_device = devices.front();
}

void Context::createLogicalDevice()
{
	queue_family_indices = queryQueueFamilyIndices();

	std::array layers = {"VK_LAYER_KHRONOS_validation"};
	std::array extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos{};

	std::set<uint32_t> unique_queue_families = {
	    queue_family_indices.graphics_family.value(),
	    queue_family_indices.present_family.value(),
	};

	float queue_priority = 1.0f;
	for (auto family : unique_queue_families) {
		vk::DeviceQueueCreateInfo queue_create_info{};
		queue_create_info.setPQueuePriorities(&queue_priority)
		    .setQueueCount(1)
		    .setQueueFamilyIndex(family);
		queue_create_infos.push_back(std::move(queue_create_info));
	}

	vk::DeviceCreateInfo create_info{};
	create_info.setQueueCreateInfos(queue_create_infos)
	    .setEnabledLayerCount(layers.size())
	    .setPEnabledLayerNames(layers)
	    .setEnabledExtensionCount(extensions.size())
	    .setPEnabledExtensionNames(extensions);

	logical_device = physical_device.createDevice(create_info);

	graphics_queue = logical_device.getQueue(queue_family_indices.graphics_family.value(), 0);
	present_queue = logical_device.getQueue(queue_family_indices.present_family.value(), 0);
}

void Context::createCommandPools()
{
	graphics_command_pool = std::make_unique<CommandPool>(
	    *this,
	    queue_family_indices.graphics_family.value(),
	    vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	transfer_command_pool = std::make_unique<CommandPool>(
	    *this,
	    queue_family_indices.graphics_family.value(),
	    vk::CommandPoolCreateFlagBits::eTransient);
}

QueueFamilyIndices Context::queryQueueFamilyIndices() const
{
	QueueFamilyIndices queue_family_indices;

	auto properties = physical_device.getQueueFamilyProperties();
	for (int i = 0; i < properties.size(); i++) {
		const auto& property = properties[i];

		if (property.queueFlags & vk::QueueFlagBits::eGraphics)
			queue_family_indices.graphics_family = i;

		if (physical_device.getSurfaceSupportKHR(i, surface))
			queue_family_indices.present_family = i;

		if (queue_family_indices)
			break;
	}

	return queue_family_indices;
}

void Context::execute(std::function<void(vk::CommandBuffer)> func)
{
	auto command_buffer = transfer_command_pool->allocate();

	CommandPool::begin(command_buffer);
	func(command_buffer);
	CommandPool::end(command_buffer);

	Fence fence(*this, false);
	submit(command_buffer, {}, {}, {}, fence.get());
	fence.wait();

	transfer_command_pool->free(command_buffer);
}

void Context::submit(vk::CommandBuffer                       command,
                     std::span<const vk::Semaphore>          wait_semaphores,
                     std::span<const vk::Semaphore>          signal_semaphores,
                     std::span<const vk::PipelineStageFlags> wait_stages,
                     vk::Fence                               fence)
{
	vk::SubmitInfo submit_info{};
	submit_info.setCommandBuffers(command)
	    .setWaitSemaphores(wait_semaphores)
	    .setSignalSemaphores(signal_semaphores)
	    .setWaitDstStageMask(wait_stages);

	graphics_queue.submit(submit_info, fence);
}

void Context::present(std::span<const uint32_t>         image_indices,
                      std::span<const vk::SwapchainKHR> swap_chains,
                      std::span<const vk::Semaphore>    wait_semaphores)
{
	vk::PresentInfoKHR present_info{};
	present_info.setImageIndices(image_indices)
	    .setSwapchains(swap_chains)
	    .setWaitSemaphores(wait_semaphores);

	if (present_queue.presentKHR(present_info) != vk::Result::eSuccess)
		throw std::runtime_error("Failed to present swap chain image");
}

vk::Instance Context::getInstance() const
{
	return instance;
}

vk::SurfaceKHR Context::getSurface() const
{
	return surface;
}

vk::PhysicalDevice Context::getPhysicalDevice() const
{
	return physical_device;
}

vk::Device Context::getLogicalDevice() const
{
	return logical_device;
}

vk::Queue Context::getGraphicsQueue() const
{
	return graphics_queue;
}

vk::Queue Context::getPresentQueue() const
{
	return present_queue;
}

uint32_t Context::getGraphicsQueueIndex() const
{
	return queue_family_indices.graphics_family.value();
}

uint32_t Context::getPresentQueueIndex() const
{
	return queue_family_indices.present_family.value();
}

CommandPool& Context::getGraphicsCommandPool() const
{
	return *graphics_command_pool;
}

CommandPool& Context::getTransferCommandPool() const
{
	return *transfer_command_pool;
}

QueueFamilyIndices::operator bool() const
{
	return graphics_family.has_value() && present_family.has_value();
}

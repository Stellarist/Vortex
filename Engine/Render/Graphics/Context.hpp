#pragma once

#include <functional>

#include <vulkan/vulkan.hpp>

#include "Platform/Window.hpp"

class Buffer;
class CommandPool;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	operator bool() const;
};

class Context {
private:
	vk::Instance       instance;
	vk::SurfaceKHR     surface;
	vk::PhysicalDevice physical_device;
	vk::Device         logical_device;
	vk::Queue          graphics_queue;
	vk::Queue          present_queue;

	std::unique_ptr<CommandPool> graphics_command_pool;
	std::unique_ptr<CommandPool> transfer_command_pool;

	Window* window{};

	QueueFamilyIndices queue_family_indices;

	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createCommandPools();

	QueueFamilyIndices queryQueueFamilyIndices() const;

public:
	Context(Window& window);

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;

	Context(Context&&) noexcept = default;
	Context& operator=(Context&&) noexcept = default;

	~Context();

	void execute(std::function<void(vk::CommandBuffer)> func);

	void submit(vk::CommandBuffer                       command,
	            std::span<const vk::Semaphore>          wait_semaphores = {},
	            std::span<const vk::Semaphore>          signal_semaphores = {},
	            std::span<const vk::PipelineStageFlags> wait_stages = {},
	            vk::Fence                               fence = {});

	void present(std::span<const uint32_t>         image_indices,
	             std::span<const vk::SwapchainKHR> swap_chains = {},
	             std::span<const vk::Semaphore>    wait_semaphores = {});

	vk::Instance       getInstance() const;
	vk::SurfaceKHR     getSurface() const;
	vk::PhysicalDevice getPhysicalDevice() const;
	vk::Device         getLogicalDevice() const;
	vk::Queue          getGraphicsQueue() const;
	vk::Queue          getPresentQueue() const;
	uint32_t           getGraphicsQueueIndex() const;
	uint32_t           getPresentQueueIndex() const;
	CommandPool&       getGraphicsCommandPool() const;
	CommandPool&       getTransferCommandPool() const;
};

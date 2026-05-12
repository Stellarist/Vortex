#pragma once

#include <functional>

#include <vulkan/vulkan.hpp>

#include "Editor/Window.hpp"

class VulkanBuffer;
class VulkanDevice;
class VulkanSwapChain;
class VulkanCommandBuffer;
class VulkanCommandPool;
class VulkanSemaphore;
class VulkanFence;

class VulkanContext {
private:
	vk::Instance   instance;
	vk::SurfaceKHR surface;

	std::unique_ptr<VulkanDevice>      device;
	std::unique_ptr<VulkanSwapChain>   swap_chain;
	std::unique_ptr<VulkanCommandPool> graphics_command_pool;
	std::unique_ptr<VulkanCommandPool> transfer_command_pool;

	Window* window{};

	void createInstance();
	void createSurface();
	void createDevice();
	void createSwapChain();
	void createCommandPools();

	std::vector<const char*> requestExtensions();
	std::vector<const char*> requestLayers();

public:
	VulkanContext(Window& window);
	~VulkanContext();

	VulkanContext(const VulkanContext&) = delete;
	VulkanContext& operator=(const VulkanContext&) = delete;

	VulkanContext(VulkanContext&&) noexcept = default;
	VulkanContext& operator=(VulkanContext&&) noexcept = default;

	void execute(std::function<void(VulkanCommandBuffer)> func);

	void submit(const std::vector<VulkanCommandBuffer>& cmds, VulkanFence* fence = {},
	    const std::vector<VulkanSemaphore*>&             waits = {},
	    const std::vector<VulkanSemaphore*>&             signals = {},
	    const std::vector<vk::PipelineStageFlags>& stages = {});

	void present(const std::vector<uint32_t>& images,
	    const std::vector<VulkanSemaphore*>&        waits = {});

	vk::Instance   getInstance() const;
	vk::SurfaceKHR getSurface() const;

	VulkanDevice&      getDevice() const;
	VulkanSwapChain&   getSwapChain() const;
	VulkanCommandPool& getGraphicsCommandPool() const;
	VulkanCommandPool& getTransferCommandPool() const;
};

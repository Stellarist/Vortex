#pragma once

#include <functional>

#include <vulkan/vulkan.hpp>

#include "Platform/Window.hpp"

class Buffer;
class Device;
class SwapChain;
class CommandBuffer;
class CommandPool;
class Semaphore;
class Fence;

class Context {
private:
	vk::Instance   instance;
	vk::SurfaceKHR surface;

	std::unique_ptr<Device>      device;
	std::unique_ptr<SwapChain>   swap_chain;
	std::unique_ptr<CommandPool> graphics_command_pool;
	std::unique_ptr<CommandPool> transfer_command_pool;

	Window* window{};

	void createInstance();
	void createSurface();
	void createDevice();
	void createSwapChain();
	void createCommandPools();

	std::vector<const char*> requestExtensions();
	std::vector<const char*> requestLayers();

public:
	Context(Window& window);
	~Context();

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;

	Context(Context&&) noexcept = default;
	Context& operator=(Context&&) noexcept = default;

	void execute(std::function<void(CommandBuffer)> func);

	void submit(const std::vector<CommandBuffer>& cmds, Fence* fence = {},
	    const std::vector<Semaphore*>&             waits = {},
	    const std::vector<Semaphore*>&             signals = {},
	    const std::vector<vk::PipelineStageFlags>& stages = {});

	void present(const std::vector<uint32_t>& images,
	    const std::vector<Semaphore*>&        waits = {});

	vk::Instance   getInstance() const;
	vk::SurfaceKHR getSurface() const;

	Device&      getDevice() const;
	SwapChain&   getSwapChain() const;
	CommandPool& getGraphicsCommandPool() const;
	CommandPool& getTransferCommandPool() const;
};

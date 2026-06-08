#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "Editor/Window.hpp"
#include "Runtime/Render/RHI/RHIDevice.hpp"

class VulkanDevice;
class VulkanContext;

struct VulkanQueueIndices {
	std::optional<uint32_t> graphics_family{};
};

struct VulkanSwapchainImages {
	std::vector<vk::Image>     images{};
	std::vector<vk::ImageView> image_views{};
};

class VulkanFrame {
private:
	std::vector<std::unique_ptr<RHICommandList>> frame_commands{};
	std::vector<std::unique_ptr<RHITexture>>     backbuffers{};

	std::vector<vk::Semaphore> image_available_semaphores{};
	std::vector<vk::Semaphore> render_finished_semaphores{};
	std::vector<vk::Fence>     in_flight_fences{};

	uint32_t current_frame{};
	uint32_t image_index{};

	VulkanContext& context;

	static constexpr uint32_t FRAMES_IN_FLIGHT = 2;

	friend class VulkanContext;

	static void transitionSwapchainImage(vk::CommandBuffer command, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout);
	static void blitToSwapchain(vk::CommandBuffer command, vk::Image src, vk::Image dst, const RHIExtent& extent);

public:
	VulkanFrame(VulkanContext& context);
	~VulkanFrame();

	void blit();
	void submit();
	void present();
	void advance();

	RHICommandList& getCommand() { return *frame_commands[current_frame]; }
	RHITexture&     getBackbuffer() { return *backbuffers[image_index]; }
};

class VulkanContext : public RHIContext {
private:
	RHIContextDesc desc{};

	vk::Instance       instance{};
	vk::SurfaceKHR     surface{};
	vk::PhysicalDevice physical_device{};
	vk::SwapchainKHR   swapchain{};

	std::unique_ptr<VulkanDevice> device{};
	std::unique_ptr<VulkanFrame>  frame{};

	VulkanQueueIndices    queue_indices{};
	VulkanSwapchainImages swapchain_images{};

	Window* window{};

public:
	VulkanContext(Window& window);
	~VulkanContext();

	void beginFrame() override;
	void endFrame() override;

	RHICommandList& getCommand() override { return frame->getCommand(); }
	RHITexture&     getBackbuffer() override { return frame->getBackbuffer(); }

	RHIDevice& getDevice() override;

	RHIFormat getFormat() const override { return desc.format; }
	RHIExtent getExtent() const override { return desc.extent; }

	VulkanFrame* getFrame() { return frame.get(); }

	vk::Instance       getInstance() const { return instance; }
	vk::SurfaceKHR     getSurface() const { return surface; }
	vk::PhysicalDevice getPhysicalDevice() const { return physical_device; }
	vk::SwapchainKHR   getSwapchain() const { return swapchain; }

	const VulkanQueueIndices&    getQueueIndices() const { return queue_indices; }
	const VulkanSwapchainImages& getSwapchainImages() const { return swapchain_images; }
};

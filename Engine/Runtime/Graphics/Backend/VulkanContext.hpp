export module Runtime.Graphics:VulkanContext;

import vulkan;
import Core;
import Editor.Window;
import :RHIDevice;
import :RHIContext;

export namespace Vortex {

class VulkanDevice;
class VulkanContext;

struct VulkanQueueIndices {
	std::optional<uint32> graphics_family{};
};

struct VulkanSwapchainImages {
	std::vector<vk::Image>     images{};
	std::vector<vk::ImageView> image_views{};
};

class VulkanFrame {
private:
	std::vector<RHIRef<RHICommandList>> frame_commands{};
	std::vector<RHIRef<RHITexture>>     backbuffers{};
	std::vector<RHIRef<RHITextureView>> backbuffer_views{};

	std::vector<vk::Semaphore> image_available_semaphores{};
	std::vector<vk::Semaphore> render_finished_semaphores{};
	std::vector<vk::Fence>     in_flight_fences{};

	uint32 current_frame{};
	uint32 image_index{};

	VulkanContext& context;

	static constexpr uint32 FRAMES_IN_FLIGHT = 2;

	static void transitionSwapchainImage(vk::CommandBuffer command, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout);
	static void blitToSwapchain(vk::CommandBuffer command, vk::Image src, vk::Image dst, const RHIExtent& extent);

	friend class VulkanContext;

public:
	VulkanFrame(VulkanContext& context);
	~VulkanFrame();

	void blit();
	void submit();
	void present();
	void advance();

	RHICommandList& getCommand() noexcept { return *frame_commands[current_frame]; }
	RHITexture&     getBackbuffer() noexcept { return *backbuffers[image_index]; }
	RHITextureView& getBackbufferView() noexcept { return *backbuffer_views[image_index]; }
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
	~VulkanContext() noexcept override;

	void beginFrame() override;
	void endFrame() override;

	RHICommandList& getCommand() noexcept override { return frame->getCommand(); }
	RHITexture&     getBackbuffer() noexcept override { return frame->getBackbuffer(); }
	RHITextureView& getBackbufferView() noexcept override { return frame->getBackbufferView(); }

	RHIDevice& getDevice() noexcept override;

	RHIFormat getFormat() const noexcept override { return desc.format; }
	RHIExtent getExtent() const noexcept override { return desc.extent; }

	VulkanFrame* getFrame() noexcept { return frame.get(); }

	vk::Instance       getInstance() const noexcept { return instance; }
	vk::SurfaceKHR     getSurface() const noexcept { return surface; }
	vk::PhysicalDevice getPhysicalDevice() const noexcept { return physical_device; }
	vk::SwapchainKHR   getSwapchain() const noexcept { return swapchain; }

	const VulkanQueueIndices&    getQueueIndices() const noexcept { return queue_indices; }
	const VulkanSwapchainImages& getSwapchainImages() const noexcept { return swapchain_images; }
};

}        // namespace Vortex

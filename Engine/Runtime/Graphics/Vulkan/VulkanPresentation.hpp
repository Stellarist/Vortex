export module Runtime.Graphics:Vulkan.Presentation;

import vulkan;
import Core;
import :RHI.Device;
import :RHI.Context;

namespace Vortex {

class VulkanDevice;

class VulkanPresentation {
private:
	struct SubmissionSlot {
		RHIRef<RHICommandList> command{};
		vk::Semaphore          image_available{};
		vk::Fence              in_flight{};
	};

	struct PresentImage {
		vk::Image              image{};
		vk::Semaphore          render_finished{};
		RHIRef<RHITexture>     backbuffer{};
		RHIRef<RHITextureView> backbuffer_view{};
	};

	static constexpr uint32 MAX_FRAMES_IN_FLIGHT = 2;

	vk::PhysicalDevice physical_device{};
	vk::SurfaceKHR     surface{};
	vk::SwapchainKHR   swapchain{};

	RHIContextDesc desc{};

	std::array<SubmissionSlot, MAX_FRAMES_IN_FLIGHT> submission_slots{};
	std::vector<PresentImage>                        present_images{};

	uint32 submission_slot_index{};
	uint32 acquired_image_index{};
	uint32 queue_family{};

	bool frame_active{};
	bool swapchain_recreate_pending{};

	VulkanDevice& device;

	void createSwapchain(uint32 width, uint32 height);
	bool recreateSwapchain(const RHIExtent& extent);
	void destroySwapchain() noexcept;

	void createResources(std::span<const VkImage> images);
	void destroyResources() noexcept;

public:
	VulkanPresentation(
	    VulkanDevice&      device,
	    vk::PhysicalDevice physical_device,
	    vk::SurfaceKHR     surface,
	    uint32             queue_family,
	    const RHIExtent&   initial_extent);
	~VulkanPresentation() noexcept;

	bool beginFrame(const RHIExtent& surface_extent);
	void endFrame();

	RHICommandList& getCommand() noexcept { return *submission_slots[submission_slot_index].command; }
	RHITexture& getBackbuffer() noexcept { return *present_images[acquired_image_index].backbuffer; }
	RHITextureView& getBackbufferView() noexcept { return *present_images[acquired_image_index].backbuffer_view; }

	RHIFormat getFormat() const noexcept { return desc.format; }
	RHIExtent getExtent() const noexcept { return desc.extent; }
};

}        // namespace Vortex

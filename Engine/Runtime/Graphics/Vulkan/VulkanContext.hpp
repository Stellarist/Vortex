export module Runtime.Graphics:Vulkan.Context;

import vulkan;
import Core;
import Editor.Window;
import :RHI.Device;
import :RHI.Context;

export namespace Vortex {

class VulkanDevice;
class VulkanPresentation;

struct VulkanQueueIndices {
	std::optional<uint32> graphics_family{};
};

class VulkanContext : public RHIContext {
private:
	vk::Instance               instance{};
	vk::DebugUtilsMessengerEXT debug_messenger{};
	vk::SurfaceKHR             surface{};
	vk::PhysicalDevice         physical_device{};

	std::unique_ptr<VulkanDevice>       device{};
	std::unique_ptr<VulkanPresentation> presentation{};

	VulkanQueueIndices queue_indices{};

	std::atomic_uint32_t validation_error_count{};

	Window* window{};

public:
	VulkanContext(Window& window);
	~VulkanContext() noexcept override;

	bool beginFrame() override;
	void endFrame() override;

	RHICommandList& getCommand() noexcept override;
	RHITexture& getBackbuffer() noexcept override;
	RHITextureView& getBackbufferView() noexcept override;

	RHIDevice& getDevice() noexcept override;

	RHIFormat getFormat() const noexcept override;
	RHIExtent getExtent() const noexcept override;

	uint32 getValidationErrorCount() const noexcept { return validation_error_count.load(std::memory_order_relaxed); }

	vk::Instance getInstance() const noexcept { return instance; }
	vk::SurfaceKHR getSurface() const noexcept { return surface; }
	vk::PhysicalDevice getPhysicalDevice() const noexcept { return physical_device; }

	const VulkanQueueIndices& getQueueIndices() const noexcept { return queue_indices; }
};

}        // namespace Vortex

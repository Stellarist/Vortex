module;

#include <vulkan/vulkan.h>

export module Runtime.Vulkan:Validation;

import vulkan;
import Core;

export namespace Vortex {

class VulkanValidationLayer {
private:
	vk::Instance instance{};
	vk::DebugUtilsMessengerEXT messenger{};

	std::atomic_uint32_t error_count{};
	bool available{};

public:
	VulkanValidationLayer();
	~VulkanValidationLayer() noexcept;

	void attach(vk::Instance instance, vk::DebugUtilsMessengerEXT messenger) noexcept;
	void reset() noexcept;

	PFN_vkDebugUtilsMessengerCallbackEXT getCallback() const noexcept;
	void* getUserData() noexcept { return &error_count; }

	bool isAvailable() const noexcept { return available; }
	bool isEnabled() const noexcept { return static_cast<bool>(messenger); }
	uint32 getErrorCount() const noexcept { return error_count.load(std::memory_order_relaxed); }
};

}        // namespace Vortex

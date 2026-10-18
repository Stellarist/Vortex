module;

#include <VkBootstrap.h>

module Runtime.Vulkan;

import vulkan;

namespace Vortex {

#if VDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL vortexValidationCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) noexcept
{
	if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0 && user_data)
		static_cast<std::atomic_uint32_t*>(user_data)->fetch_add(1, std::memory_order_relaxed);

	static_cast<void>(message_type);
	const std::string_view message = callback_data && callback_data->pMessage ?
	    callback_data->pMessage :
	    "Vulkan validation produced an empty message";

	try {
		if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
			LOG(Error, "Vulkan validation: {}", message);
		else if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
			LOG(Warn, "Vulkan validation: {}", message);
		else
			LOG(Debug, "Vulkan validation: {}", message);
	} catch (...) {
	}
	return VK_FALSE;
}
#endif

VulkanValidationLayer::VulkanValidationLayer()
{
#if VDEBUG
	available = vkb::SystemInfo::get_system_info().value().validation_layers_available;
	if (!available)
		LOG(Warn, "Vulkan validation layer unavailable; install Vulkan validation layers to enable validation");
#endif
}

VulkanValidationLayer::~VulkanValidationLayer() noexcept
{
	reset();
}

void VulkanValidationLayer::reset() noexcept
{
#if VDEBUG
	if (messenger)
		vkb::destroy_debug_utils_messenger(static_cast<VkInstance>(instance), static_cast<VkDebugUtilsMessengerEXT>(messenger));
#endif
	messenger = vk::DebugUtilsMessengerEXT{};
	instance = vk::Instance{};
}

void VulkanValidationLayer::attach(vk::Instance instance, vk::DebugUtilsMessengerEXT messenger) noexcept
{
#if VDEBUG
	this->instance = instance;
	this->messenger = messenger;
#else
	static_cast<void>(instance);
	static_cast<void>(messenger);
#endif
}

PFN_vkDebugUtilsMessengerCallbackEXT VulkanValidationLayer::getCallback() const noexcept
{
#if VDEBUG
	return vortexValidationCallback;
#else
	return nullptr;
#endif
}

}        // namespace Vortex

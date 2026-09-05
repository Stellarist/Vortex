module;

#include <VkBootstrap.h>
#include <SDL3/SDL_vulkan.h>

module Runtime.Graphics;

import vulkan;

namespace Vortex {

#if VDEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL vortexValidationCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
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
		// Never allow logging failures to escape through a Vulkan C callback.
	}
	return VK_FALSE;
}
#endif

VulkanContext::VulkanContext(Window& window) :
    window(&window)
{
	vkb::InstanceBuilder instance_builder{};
	instance_builder.set_app_name("Vortex")
	    .require_api_version(1, 4, 0);
#if VDEBUG
	instance_builder.request_validation_layers()
	    .set_debug_callback(vortexValidationCallback)
	    .set_debug_callback_user_data_pointer(&validation_error_count);
#endif

	auto vkb_instance = instance_builder.build().value();
	auto vk_surface = VkSurfaceKHR{};
	CHECK(SDL_Vulkan_CreateSurface(window.get(), vkb_instance.instance, nullptr, &vk_surface),
	    "Failed to create Vulkan surface: {}", SDL_GetError());

	vk::PhysicalDeviceVulkan11Features features11{};
	features11.setShaderDrawParameters(true);

	vk::PhysicalDeviceVulkan12Features features12{};
	features12.setTimelineSemaphore(true);

	vk::PhysicalDeviceVulkan13Features features13{};
	features13.setSynchronization2(true)
	    .setDynamicRendering(true);

	auto vkb_physical_device =
	    vkb::PhysicalDeviceSelector(vkb_instance)
	        .set_required_features_11(features11)
	        .set_required_features_12(features12)
	        .set_required_features_13(features13)
	        .set_surface(vk_surface)
	        .select()
	        .value();

	auto vkb_device =
	    vkb::DeviceBuilder(vkb_physical_device)
	        .build()
	        .value();

	instance = std::move(vkb_instance);
	debug_messenger = vkb_instance.debug_messenger;
	surface = std::move(vk_surface);
	physical_device = std::move(vkb_physical_device);

	queue_indices.graphics_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

	device = std::make_unique<VulkanDevice>(*this, vkb_device.device);

	uint32 pixel_width{};
	uint32 pixel_height{};
	window.getPixelSize(pixel_width, pixel_height);
	presentation = std::make_unique<VulkanPresentation>(
	    *device,
	    physical_device,
	    surface,
	    queue_indices.graphics_family.value(),
	    RHIExtent{pixel_width, pixel_height});

	const auto properties = physical_device.getProperties();
	LOG("Vulkan initialized on '{}' (graphics queue {}, {}x{})",
	    properties.deviceName.data(), queue_indices.graphics_family.value(),
	    pixel_width, pixel_height);
}

VulkanContext::~VulkanContext() noexcept
{
	if (device)
		device->waitIdle();

	presentation.reset();
	device.reset();

	instance.destroySurfaceKHR(surface);
	if (debug_messenger) {
		vkb::destroy_debug_utils_messenger(
		    static_cast<VkInstance>(instance),
		    static_cast<VkDebugUtilsMessengerEXT>(debug_messenger));
		debug_messenger = vk::DebugUtilsMessengerEXT{};
	}

	instance.destroy();
	LOG(Debug, "Vulkan context shut down");
}

RHIDevice& VulkanContext::getDevice() noexcept
{
	return *device;
}

bool VulkanContext::beginFrame()
{
	uint32 pixel_width{};
	uint32 pixel_height{};
	window->getPixelSize(pixel_width, pixel_height);

	if (window->isMinimized() || pixel_width == 0 || pixel_height == 0)
		return false;

	return presentation->beginFrame(RHIExtent{pixel_width, pixel_height});
}

void VulkanContext::endFrame()
{
	presentation->endFrame();
}

RHICommandList& VulkanContext::getCommand() noexcept
{
	return presentation->getCommand();
}

RHITexture& VulkanContext::getBackbuffer() noexcept
{
	return presentation->getBackbuffer();
}

RHITextureView& VulkanContext::getBackbufferView() noexcept
{
	return presentation->getBackbufferView();
}

RHIFormat VulkanContext::getFormat() const noexcept
{
	return presentation->getFormat();
}

RHIExtent VulkanContext::getExtent() const noexcept
{
	return presentation->getExtent();
}

}        // namespace Vortex

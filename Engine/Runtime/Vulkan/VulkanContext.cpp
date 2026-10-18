module;

#include <VkBootstrap.h>
#include <SDL3/SDL_vulkan.h>

module Runtime.Vulkan;

import vulkan;

namespace Vortex {

VulkanContext::VulkanContext(Window& window) :
    window(&window)
{
	vkb::InstanceBuilder instance_builder{};
	instance_builder.set_app_name("Vortex")
	    .require_api_version(1, 4, 0);
#if VDEBUG
	if (validation.isAvailable()) {
		instance_builder.enable_validation_layers()
		    .set_debug_callback(validation.getCallback())
		    .set_debug_callback_user_data_pointer(validation.getUserData());
	}
#endif

	auto vkb_instance = instance_builder.build().value();
	instance = vkb_instance.instance;
	validation.attach(instance, vkb_instance.debug_messenger);

	auto vk_surface = VkSurfaceKHR{};
	CHECK(SDL_Vulkan_CreateSurface(window.get(), vkb_instance.instance, nullptr, &vk_surface),
	    "Failed to create Vulkan surface: {}", SDL_GetError());

	vk::PhysicalDeviceFeatures features{};
	features.setShaderSampledImageArrayDynamicIndexing(true)
	    .setShaderStorageImageArrayDynamicIndexing(true)
	    .setShaderStorageBufferArrayDynamicIndexing(true)
	    .setShaderUniformBufferArrayDynamicIndexing(true);

	vk::PhysicalDeviceVulkan11Features features11{};
	features11.setShaderDrawParameters(true);

	vk::PhysicalDeviceVulkan12Features features12{};
	features12.setTimelineSemaphore(true)
	    .setRuntimeDescriptorArray(true)
	    .setDescriptorBindingPartiallyBound(true)
	    .setDescriptorBindingUpdateUnusedWhilePending(true)
	    .setDescriptorBindingSampledImageUpdateAfterBind(true)
	    .setDescriptorBindingStorageImageUpdateAfterBind(true)
	    .setDescriptorBindingStorageBufferUpdateAfterBind(true)
	    .setDescriptorBindingUniformBufferUpdateAfterBind(true)
	    .setDescriptorBindingUniformTexelBufferUpdateAfterBind(true)
	    .setDescriptorBindingStorageTexelBufferUpdateAfterBind(true)
	    .setShaderSampledImageArrayNonUniformIndexing(true)
	    .setShaderStorageImageArrayNonUniformIndexing(true)
	    .setShaderStorageBufferArrayNonUniformIndexing(true)
	    .setShaderUniformBufferArrayNonUniformIndexing(true)
	    .setShaderUniformTexelBufferArrayNonUniformIndexing(true)
	    .setShaderStorageTexelBufferArrayNonUniformIndexing(true)
	    .setShaderUniformTexelBufferArrayDynamicIndexing(true)
	    .setShaderStorageTexelBufferArrayDynamicIndexing(true);

	vk::PhysicalDeviceVulkan13Features features13{};
	features13.setSynchronization2(true)
	    .setDynamicRendering(true);

	vk::PhysicalDeviceVulkan14Features features14{};
	features14.setMaintenance5(true)
	    .setMaintenance6(true);

	auto vkb_physical_device =
	    vkb::PhysicalDeviceSelector(vkb_instance)
	        .set_minimum_version(1, 4)
	        .set_required_features(features)
	        .set_required_features_11(features11)
	        .set_required_features_12(features12)
	        .set_required_features_13(features13)
	        .set_required_features_14(features14)
	        .set_surface(vk_surface)
	        .select()
	        .value();

	auto vkb_device =
	    vkb::DeviceBuilder(vkb_physical_device)
	        .build()
	        .value();

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
	LOG("Vulkan initialized on '{}' (graphics queue {}, {}x{}, validation {})",
	    properties.deviceName.data(), queue_indices.graphics_family.value(),
	    pixel_width, pixel_height, validation.isEnabled() ? "enabled" : "disabled");
}

VulkanContext::~VulkanContext() noexcept
{
	if (device)
		device->waitIdle();

	presentation.reset();
	device.reset();

	instance.destroySurfaceKHR(surface);
	validation.reset();

	instance.destroy();
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

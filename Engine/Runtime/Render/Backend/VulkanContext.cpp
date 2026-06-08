#include "VulkanContext.hpp"

#include <VkBootstrap.h>

#include "VulkanTypes.hpp"
#include "VulkanResources.hpp"
#include "VulkanDevice.hpp"
#include "VulkanQueue.hpp"

VulkanFrame::VulkanFrame(VulkanContext& context) :
    context(context)
{
	RHITextureDesc backbuffer_desc{};
	backbuffer_desc.setWidth(context.getExtent().width)
	    .setHeight(context.getExtent().height)
	    .setFormat(context.getFormat())
	    .setUsage(RHITextureUsage::RenderTarget | RHITextureUsage::CopySrc | RHITextureUsage::CopyDst);

	vk::SemaphoreCreateInfo semaphore_info{};
	vk::FenceCreateInfo     fence_info{};
	fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);

	auto& device = static_cast<VulkanDevice&>(context.getDevice());

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		frame_commands.push_back(device.createCommandList(RHICommandListDesc{}));
		image_available_semaphores.push_back(device.getHandle().createSemaphore(semaphore_info));
		in_flight_fences.push_back(device.getHandle().createFence(fence_info));
	}

	for (size_t i = 0; i < context.getSwapchainImages().images.size(); i++) {
		backbuffers.push_back(device.createTexture(backbuffer_desc));
		render_finished_semaphores.push_back(device.getHandle().createSemaphore(semaphore_info));
	}
}

VulkanFrame::~VulkanFrame()
{
	auto& device = static_cast<VulkanDevice&>(context.getDevice());

	for (auto fence : in_flight_fences)
		device.getHandle().destroyFence(fence);

	for (auto semaphore : image_available_semaphores)
		device.getHandle().destroySemaphore(semaphore);

	for (auto semaphore : render_finished_semaphores)
		device.getHandle().destroySemaphore(semaphore);

	frame_commands.clear();
	backbuffers.clear();
}

void VulkanFrame::blit()
{
	auto* vk_command_list = static_cast<VulkanCommandList*>(&getCommand());
	auto* vk_backbuffer = static_cast<VulkanTexture*>(&getBackbuffer());
	auto  vk_command_buffer = vk_command_list->getCurrentCommand()->getHandle();
	auto  swapchain_image = context.getSwapchainImages().images[image_index];

	vk_command_list->transitionTexture(&getBackbuffer(), CopySrc);

	transitionSwapchainImage(vk_command_buffer, swapchain_image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	blitToSwapchain(vk_command_buffer, vk_backbuffer->getHandle(), swapchain_image, context.getExtent());
	transitionSwapchainImage(vk_command_buffer, swapchain_image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);
}

void VulkanFrame::submit()
{
	auto& command = getCommand();
	auto& device = static_cast<VulkanDevice&>(context.getDevice());
	auto* vk_command_list = static_cast<VulkanCommandList*>(&command);

	command.close();

	auto wait_semaphore = image_available_semaphores[current_frame];
	auto signal_semaphore = render_finished_semaphores[image_index];
	auto fence = in_flight_fences[current_frame];
	auto wait_stage = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eTransfer);

	device.getQueue().submit(vk_command_list,
	    std::span<const vk::Semaphore>(&wait_semaphore, 1),
	    std::span<const vk::PipelineStageFlags>(&wait_stage, 1),
	    std::span<const vk::Semaphore>(&signal_semaphore, 1),
	    fence);
}

void VulkanFrame::present()
{
	auto& device = static_cast<VulkanDevice&>(context.getDevice());
	auto  signal_semaphore = render_finished_semaphores[image_index];
	auto  swapchain = context.getSwapchain();

	vk::PresentInfoKHR present_info{};
	present_info.setWaitSemaphores(signal_semaphore)
	    .setSwapchains(swapchain)
	    .setImageIndices(image_index);

	if (device.getQueue().getHandle().presentKHR(present_info) != vk::Result::eSuccess)
		throw std::runtime_error("Failed to present Vulkan swapchain image");
}

void VulkanFrame::advance()
{
	current_frame = (current_frame + 1) % FRAMES_IN_FLIGHT;
}


VulkanContext::VulkanContext(Window& window) :
    window(&window)
{
	desc = RHIContextDesc{
	    .api = RHIAPI::Vulkan,
	    .extent = {
	        window.getWidth(),
	        window.getHeight(),
	    },
	    .format = RHIFormat::RGBA8_SRGB,
	};

	auto vkb_instance =
	    vkb::InstanceBuilder()
	        .set_app_name("Vortex")
	        .require_api_version(1, 4, 0)
	        .request_validation_layers()
	        .build()
	        .value();

	auto vk_surface = VkSurfaceKHR{};
	if (!SDL_Vulkan_CreateSurface(window.get(), vkb_instance.instance, nullptr, &vk_surface))
		throw std::runtime_error("Failed to create Vulkan surface");

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

	auto vkb_swapchain =
	    vkb::SwapchainBuilder(vkb_physical_device, vkb_device, vk_surface)
	        .set_desired_format({static_cast<VkFormat>(toVkFormat(getFormat())), {}})
	        .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
	        .set_desired_extent(getExtent().width, getExtent().height)
	        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	        .build()
	        .value();

	auto vkb_images = vkb_swapchain.get_images().value();
	auto vkb_image_views = vkb_swapchain.get_image_views().value();

	instance = std::move(vkb_instance);
	surface = std::move(vk_surface);
	physical_device = std::move(vkb_physical_device);
	swapchain = std::move(vkb_swapchain);

	queue_indices.graphics_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
	swapchain_images.images.assign(vkb_images.begin(), vkb_images.end());
	swapchain_images.image_views.assign(vkb_image_views.begin(), vkb_image_views.end());

	device = std::make_unique<VulkanDevice>(*this, vkb_device.device);
	frame = std::make_unique<VulkanFrame>(*this);
}

VulkanContext::~VulkanContext()
{
	if (device)
		device->getHandle().waitIdle();

	frame.reset();

	for (auto image_view : swapchain_images.image_views)
		device->getHandle().destroyImageView(image_view);

	swapchain_images.image_views.clear();

	device->getHandle().destroySwapchainKHR(swapchain);
	device.reset();

	instance.destroySurfaceKHR(surface);
	instance.destroy();
}

RHIDevice& VulkanContext::getDevice()
{
	return *device;
}

void VulkanContext::beginFrame()
{
	auto fence = frame->in_flight_fences[frame->current_frame];
	if (device->getHandle().waitForFences(fence, true, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
		throw std::runtime_error("Failed to wait for frame fence");

	auto next_image = device->getHandle().acquireNextImageKHR(swapchain,
	    std::numeric_limits<uint64_t>::max(),
	    frame->image_available_semaphores[frame->current_frame]);
	frame->image_index = next_image.value;

	device->getHandle().resetFences(fence);
	frame->getCommand().open();
}

void VulkanContext::endFrame()
{
	frame->blit();
	frame->submit();
	frame->present();
	frame->advance();
}

void VulkanFrame::transitionSwapchainImage(vk::CommandBuffer command, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout)
{
	vk::ImageSubresourceRange range{};
	range.setAspectMask(vk::ImageAspectFlagBits::eColor)
	    .setBaseMipLevel(0)
	    .setLevelCount(1)
	    .setBaseArrayLayer(0)
	    .setLayerCount(1);

	vk::ImageMemoryBarrier2 barrier{};
	barrier.setOldLayout(old_layout)
	    .setNewLayout(new_layout)
	    .setSrcStageMask(old_layout == vk::ImageLayout::eUndefined ? vk::PipelineStageFlagBits2::eTopOfPipe : getVkPipelineStageFlags(old_layout))
	    .setDstStageMask(getVkPipelineStageFlags(new_layout))
	    .setSrcAccessMask({})
	    .setDstAccessMask({})
	    .setImage(image)
	    .setSubresourceRange(range);

	vk::DependencyInfo dependency{};
	dependency.setImageMemoryBarriers(barrier);

	command.pipelineBarrier2(dependency);
}


void VulkanFrame::blitToSwapchain(vk::CommandBuffer command, vk::Image src, vk::Image dst, const RHIExtent& extent)
{
	vk::ImageSubresourceLayers src_layers{}, dst_layers{};
	src_layers = dst_layers =
	    vk::ImageSubresourceLayers{}
	        .setAspectMask(vk::ImageAspectFlagBits::eColor)
	        .setMipLevel(0)
	        .setBaseArrayLayer(0)
	        .setLayerCount(1);

	vk::ImageBlit blit{};
	blit.setSrcSubresource(src_layers)
	    .setSrcOffsets({vk::Offset3D{0, 0, 0},
	        vk::Offset3D{static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1}})
	    .setDstSubresource(dst_layers)
	    .setDstOffsets({vk::Offset3D{0, 0, 0},
	        vk::Offset3D{static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1}});

	command.blitImage(
	    src,
	    vk::ImageLayout::eTransferSrcOptimal,
	    dst,
	    vk::ImageLayout::eTransferDstOptimal,
	    blit,
	    vk::Filter::eNearest);
}

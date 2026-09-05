module;

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

module Runtime.Vulkan;

import vulkan;

namespace Vortex {

static void transitionSwapchainImage(
    vk::CommandBuffer command,
    vk::Image image,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout)
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

static void blitToSwapchain(vk::CommandBuffer command, vk::Image source, vk::Image destination, const RHIExtent& extent)
{
	vk::ImageSubresourceLayers source_layers{};
	source_layers.setAspectMask(vk::ImageAspectFlagBits::eColor)
	    .setMipLevel(0)
	    .setBaseArrayLayer(0)
	    .setLayerCount(1);

	vk::ImageSubresourceLayers destination_layers = source_layers;

	vk::ImageBlit blit{};
	blit.setSrcSubresource(source_layers)
	    .setSrcOffsets({vk::Offset3D{0, 0, 0},
	        vk::Offset3D{static_cast<int32>(extent.width), static_cast<int32>(extent.height), 1}})
	    .setDstSubresource(destination_layers)
	    .setDstOffsets({vk::Offset3D{0, 0, 0},
	        vk::Offset3D{static_cast<int32>(extent.width), static_cast<int32>(extent.height), 1}});

	command.blitImage(
	    source,
	    vk::ImageLayout::eTransferSrcOptimal,
	    destination,
	    vk::ImageLayout::eTransferDstOptimal,
	    blit,
	    vk::Filter::eNearest);
}

VulkanPresentation::VulkanPresentation(
    VulkanDevice& device,
    vk::PhysicalDevice physical_device,
    vk::SurfaceKHR surface,
    uint32 queue_family,
    const RHIExtent& initial_extent) :
    device(device),
    physical_device(physical_device),
    surface(surface),
    queue_family(queue_family)
{
	createSwapchain(initial_extent.width, initial_extent.height);
}

VulkanPresentation::~VulkanPresentation() noexcept
{
	destroyResources();
	destroySwapchain();
}

void VulkanPresentation::createSwapchain(uint32 width, uint32 height)
{
	vkb::SwapchainBuilder builder(
	    static_cast<VkPhysicalDevice>(physical_device),
	    static_cast<VkDevice>(device.getHandle()),
	    static_cast<VkSurfaceKHR>(surface),
	    queue_family,
	    queue_family);
	builder.set_desired_format({static_cast<VkFormat>(toVkFormat(desc.format)), {}})
	    .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
	    .set_desired_extent(width, height)
	    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	if (swapchain)
		builder.set_old_swapchain(static_cast<VkSwapchainKHR>(swapchain));

	auto new_swapchain = builder.build().value();
	auto images = new_swapchain.get_images().value();

	if (swapchain)
		device.getHandle().destroySwapchainKHR(swapchain);

	swapchain = new_swapchain.swapchain;
	desc.extent = {new_swapchain.extent.width, new_swapchain.extent.height};
	swapchain_recreate_pending = false;
	createResources(images);
	LOG("Vulkan swapchain ready ({}x{}, {} images)", desc.extent.width, desc.extent.height, images.size());
}

bool VulkanPresentation::recreateSwapchain(const RHIExtent& extent)
{
	if (extent.width == 0 || extent.height == 0)
		return false;

	device.waitIdle();
	destroyResources();
	createSwapchain(extent.width, extent.height);
	return true;
}

void VulkanPresentation::destroySwapchain() noexcept
{
	if (!swapchain)
		return;

	device.getHandle().destroySwapchainKHR(swapchain);
	swapchain = vk::SwapchainKHR{};
}

void VulkanPresentation::createResources(std::span<const VkImage> images)
{
	vk::SemaphoreCreateInfo semaphore_info{};
	vk::FenceCreateInfo fence_info{};
	fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);

	for (auto& submission : submission_slots) {
		submission.command = device.createCommandList(RHICommandListDesc{});
		submission.image_available = device.getHandle().createSemaphore(semaphore_info);
		submission.in_flight = device.getHandle().createFence(fence_info);
	}

	RHITextureDesc backbuffer_desc{};
	backbuffer_desc.setWidth(desc.extent.width)
	    .setHeight(desc.extent.height)
	    .setFormat(desc.format)
	    .setUsage(RHITextureUsage::RenderTarget | RHITextureUsage::CopySource | RHITextureUsage::CopyDest);

	present_images.reserve(images.size());
	for (const auto image_handle : images) {
		PresentImage image{};
		image.image = image_handle;
		image.render_finished = device.getHandle().createSemaphore(semaphore_info);
		image.backbuffer = device.createTexture(backbuffer_desc);
		image.backbuffer_view = device.createTextureView(
		    RHITextureViewDesc{}
		        .setTexture(image.backbuffer.get())
		        .setType(RHITextureViewType::RenderTarget));
		present_images.push_back(std::move(image));
	}
}

void VulkanPresentation::destroyResources() noexcept
{
	for (auto& submission : submission_slots) {
		if (submission.in_flight)
			device.getHandle().destroyFence(submission.in_flight);
		if (submission.image_available)
			device.getHandle().destroySemaphore(submission.image_available);

		submission.command = nullptr;
		submission.image_available = vk::Semaphore{};
		submission.in_flight = vk::Fence{};
	}

	for (auto& image : present_images)
		if (image.render_finished)
			device.getHandle().destroySemaphore(image.render_finished);

	present_images.clear();
	submission_slot_index = 0;
	acquired_image_index = 0;
	frame_active = false;
}

bool VulkanPresentation::beginFrame(const RHIExtent& surface_extent)
{
	frame_active = false;

	if (surface_extent.width == 0 || surface_extent.height == 0)
		return false;

	if (swapchain_recreate_pending || desc.extent != surface_extent) {
		LOG(Debug, "Recreating Vulkan swapchain ({}x{} -> {}x{})",
		    desc.extent.width, desc.extent.height,
		    surface_extent.width, surface_extent.height);
		if (!recreateSwapchain(surface_extent))
			return false;
	}

	auto& submission = submission_slots[submission_slot_index];
	const auto wait_result = device.getHandle().waitForFences(
	    submission.in_flight,
	    true,
	    std::numeric_limits<uint64>::max());
	CHECK(wait_result == vk::Result::eSuccess, "Failed to wait for Vulkan submission fence (result {})",
	    static_cast<int32>(wait_result));

	uint32 image_index{};
	const auto result = device.getHandle().acquireNextImageKHR(
	    swapchain,
	    std::numeric_limits<uint64>::max(),
	    submission.image_available,
	    vk::Fence{},
	    &image_index);

	if (result == vk::Result::eErrorOutOfDateKHR) {
		swapchain_recreate_pending = true;
		LOG(Debug, "Vulkan swapchain acquire reported out-of-date; recreation scheduled");
		return false;
	}

	CHECK(result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR,
	    "Failed to acquire Vulkan swapchain image (result {})",
	    static_cast<int32>(result));

	acquired_image_index = image_index;
	device.getHandle().resetFences(submission.in_flight);
	submission.command->open();

	swapchain_recreate_pending = result == vk::Result::eSuboptimalKHR;
	if (swapchain_recreate_pending)
		LOG(Debug, "Vulkan swapchain acquire reported suboptimal; recreation scheduled");
	frame_active = true;
	return true;
}

void VulkanPresentation::endFrame()
{
	if (!frame_active)
		return;

	auto& submission = submission_slots[submission_slot_index];
	auto& image = present_images[acquired_image_index];
	auto& command = *submission.command;
	auto* vk_command_list = static_cast<VulkanCommandList*>(&command);
	auto* vk_backbuffer = static_cast<VulkanTexture*>(image.backbuffer.get());
	auto vk_command_buffer = vk_command_list->getCurrentCommand()->getHandle();

	vk_command_list->transitionTexture(image.backbuffer.get(), image.backbuffer->getState(), CopySource);
	transitionSwapchainImage(vk_command_buffer, image.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	blitToSwapchain(vk_command_buffer, vk_backbuffer->getHandle(), image.image, desc.extent);
	transitionSwapchainImage(vk_command_buffer, image.image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);

	command.close();

	const vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eTransfer;
	device.getQueue().submit(
	    vk_command_list,
	    std::span<const vk::Semaphore>(&submission.image_available, 1),
	    std::span<const vk::PipelineStageFlags>(&wait_stage, 1),
	    std::span<const vk::Semaphore>(&image.render_finished, 1),
	    submission.in_flight);

	vk::PresentInfoKHR present_info{};
	present_info.setWaitSemaphores(image.render_finished)
	    .setSwapchains(swapchain)
	    .setImageIndices(acquired_image_index);
	const auto result = device.getQueue().getHandle().presentKHR(present_info);

	submission_slot_index = (submission_slot_index + 1) % MAX_FRAMES_IN_FLIGHT;
	frame_active = false;

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
		swapchain_recreate_pending = true;
		LOG(Debug, "Vulkan swapchain present reported {}; recreation scheduled",
		    result == vk::Result::eErrorOutOfDateKHR ? "out-of-date" : "suboptimal");
		return;
	}

	CHECK(result == vk::Result::eSuccess, "Failed to present Vulkan swapchain image (result {})",
	    static_cast<int32>(result));
}

}        // namespace Vortex

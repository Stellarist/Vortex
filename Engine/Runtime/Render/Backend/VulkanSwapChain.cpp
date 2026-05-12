#include "VulkanSwapChain.hpp"

#include "VulkanDevice.hpp"

VulkanSwapChain::VulkanSwapChain(Window& window, VulkanContext& context) :
    window(&window),
    context(&context)
{
	create(window.getWidth(), window.getHeight());
	createImageViews();
}

VulkanSwapChain::~VulkanSwapChain()
{
	destroyImageViews();
	context->getDevice().logical().destroySwapchainKHR(swap_chain);
}

void VulkanSwapChain::create(uint32_t width, uint32_t height, bool old)
{
	auto details = querySwapChainDetails(width, height);

	format = chooseSwapSurfaceFormat(details.formats);
	present = chooseSwapPresentMode(details.present_modes);
	extent = chooseSwapExtent(details.capabilities, width, height);

	image_count = std::clamp<uint32_t>(details.capabilities.minImageCount + 1,
	    details.capabilities.minImageCount,
	    details.capabilities.maxImageCount);

	vk::SwapchainCreateInfoKHR create_info{};
	create_info.setClipped(vk::True)
	    .setImageArrayLayers(1)
	    .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
	    .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
	    .setSurface(context->getSurface())
	    .setImageColorSpace(format.colorSpace)
	    .setImageFormat(format.format)
	    .setImageExtent(extent)
	    .setMinImageCount(image_count)
	    .setPreTransform(details.capabilities.currentTransform)
	    .setClipped(vk::True)
	    .setPresentMode(present)
	    .setOldSwapchain(old ? swap_chain : nullptr);

	auto graphics_index = context->getDevice().graphicsQueueIndex();
	auto present_index = context->getDevice().presentQueueIndex();
	if (present_index == graphics_index)
		create_info.setQueueFamilyIndices(present_index)
		    .setImageSharingMode(vk::SharingMode::eExclusive);
	else {
		std::array queue_families = {graphics_index, present_index};
		create_info.setQueueFamilyIndices(queue_families)
		    .setImageSharingMode(vk::SharingMode::eConcurrent);
	}

	swap_chain = context->getDevice().logical().createSwapchainKHR(create_info);

	images = context->getDevice().logical().getSwapchainImagesKHR(swap_chain);
	image_views.resize(images.size());
}

void VulkanSwapChain::recreate(uint32_t width, uint32_t height)
{
	vk::SwapchainKHR old_swap_chain = swap_chain;
	destroyImageViews();

	create(width, height, true);
	context->getDevice().logical().destroySwapchainKHR(old_swap_chain);
	createImageViews();
}

void VulkanSwapChain::createImageViews()
{
	for (int i = 0; i < image_views.size(); i++) {
		vk::ImageSubresourceRange range{};
		range.setBaseMipLevel(0)
		    .setLevelCount(1)
		    .setBaseArrayLayer(0)
		    .setLayerCount(1)
		    .setAspectMask(vk::ImageAspectFlagBits::eColor);

		vk::ComponentMapping mapping{};
		mapping.setA(vk::ComponentSwizzle::eIdentity)
		    .setR(vk::ComponentSwizzle::eIdentity)
		    .setG(vk::ComponentSwizzle::eIdentity)
		    .setB(vk::ComponentSwizzle::eIdentity);

		vk::ImageViewCreateInfo create_info{};
		create_info.setImage(images[i])
		    .setViewType(vk::ImageViewType::e2D)
		    .setFormat(format.format)
		    .setSubresourceRange(range)
		    .setComponents(mapping);

		image_views[i] = context->getDevice().logical().createImageView(create_info);
	}
}

void VulkanSwapChain::destroyImageViews()
{
	for (auto& view : image_views)
		context->getDevice().logical().destroyImageView(view);

	image_views.clear();
}

uint32_t VulkanSwapChain::acquireNextImage(vk::Semaphore semaphore, vk::Fence fence)
{
	auto [result, image_index] = context->getDevice().logical().acquireNextImageKHR(swap_chain, std::numeric_limits<uint64_t>::max(), semaphore, fence);
	return result == vk::Result::eSuccess ? image_index : -1;
}

void VulkanSwapChain::presentImage(vk::Queue present_queue, uint32_t image_index, std::span<const vk::Semaphore> wait_semaphore)
{
	vk::PresentInfoKHR present_info{};
	present_info.setImageIndices(image_index)
	    .setSwapchains(swap_chain)
	    .setWaitSemaphores(wait_semaphore);

	if (present_queue.presentKHR(present_info) != vk::Result::eSuccess)
		throw std::runtime_error("Failed to present swap chain image");
}

VulkanSwapChainDetails VulkanSwapChain::querySwapChainDetails(uint32_t width, uint32_t height)
{
	return {
	    context->getDevice().physical().getSurfaceCapabilitiesKHR(context->getSurface()),
	    context->getDevice().physical().getSurfaceFormatsKHR(context->getSurface()),
	    context->getDevice().physical().getSurfacePresentModesKHR(context->getSurface()),
	};
}

vk::SurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat(std::span<const vk::SurfaceFormatKHR> surface_formats)
{
	auto fit = std::find_if(surface_formats.begin(), surface_formats.end(), [](const vk::SurfaceFormatKHR& format) {
		return format.format == vk::Format::eR8G8B8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
	});

	return (fit != surface_formats.end()) ? *fit : surface_formats.front();
}

vk::PresentModeKHR VulkanSwapChain::chooseSwapPresentMode(std::span<const vk::PresentModeKHR> present_modes)
{
	auto pit = std::find_if(present_modes.begin(), present_modes.end(), [](const vk::PresentModeKHR& present) {
		return present == vk::PresentModeKHR::eMailbox;
	});
	return (pit != present_modes.end()) ? *pit : vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanSwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else
		return {
		    std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		    std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
		};
}

vk::SwapchainKHR VulkanSwapChain::get() const
{
	return swap_chain;
}

vk::Extent2D VulkanSwapChain::getExtent() const
{
	return extent;
}

vk::SurfaceFormatKHR VulkanSwapChain::getSurfaceFormat() const
{
	return format;
}

vk::PresentModeKHR VulkanSwapChain::getPresentMode() const
{
	return present;
}

uint32_t VulkanSwapChain::getImageCount() const
{
	return image_count;
}

const std::vector<vk::Image>& VulkanSwapChain::getImages() const
{
	return images;
}

const std::vector<vk::ImageView>& VulkanSwapChain::getImageViews() const
{
	return image_views;
}

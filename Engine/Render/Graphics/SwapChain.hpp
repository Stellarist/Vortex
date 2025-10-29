#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"

struct SwapChainDetails {
	vk::SurfaceCapabilitiesKHR        capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR>   present_modes;
};

class SwapChain {
private:
	vk::SwapchainKHR     swap_chain;
	vk::SurfaceFormatKHR format;
	vk::PresentModeKHR   present;
	vk::Extent2D         extent;

	uint32_t                   image_count;
	std::vector<vk::Image>     images;
	std::vector<vk::ImageView> image_views;

	Window*  window{};
	Context* context{};

	void create(uint32_t width, uint32_t height, bool old = false);
	void recreate(uint32_t width, uint32_t height);

	void createImageViews();
	void destroyImageViews();

	SwapChainDetails     querySwapChainDetails(uint32_t width, uint32_t height);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::span<const vk::SurfaceFormatKHR> surface_formats);
	vk::PresentModeKHR   chooseSwapPresentMode(std::span<const vk::PresentModeKHR> present_modes);
	vk::Extent2D         chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

public:
	SwapChain(Window& window, Context& context);
	~SwapChain();

	SwapChain(const SwapChain&) = delete;
	SwapChain& operator=(const SwapChain&) = delete;

	SwapChain(SwapChain&&) noexcept = default;
	SwapChain& operator=(SwapChain&&) noexcept = default;

	uint32_t acquireNextImage(vk::Semaphore semaphore, vk::Fence fence);
	void     presentImage(vk::Queue present_queue, uint32_t image_index, std::span<const vk::Semaphore> wait_semaphore);

	vk::SwapchainKHR     get() const;
	vk::Extent2D         getExtent() const;
	vk::SurfaceFormatKHR getSurfaceFormat() const;
	vk::PresentModeKHR   getPresentMode() const;

	uint32_t                          getImageCount() const;
	const std::vector<vk::Image>&     getImages() const;
	const std::vector<vk::ImageView>& getImageViews() const;
};

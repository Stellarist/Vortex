#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanSampler.hpp"

class VulkanImage {
private:
	vk::Image        image;
	vk::ImageView    view;
	vk::DeviceMemory memory;
	vk::Format       format;

	int width{};
	int height{};
	int channels{};

	const void* data{};

	std::unique_ptr<VulkanBuffer> buffer;

	VulkanContext* context{};
	VulkanSampler* sampler{};

	uint32_t queryMemoryType(uint32_t type, vk::MemoryPropertyFlags prop_flags) const;

public:
	VulkanImage(VulkanContext& context, const uint8_t* data, uint32_t width, uint32_t height, vk::Format format = vk::Format::eR8G8B8A8Srgb);
	VulkanImage(VulkanContext& context, uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage);
	~VulkanImage();

	VulkanImage(const VulkanImage&) = delete;
	VulkanImage& operator=(const VulkanImage&) = delete;

	VulkanImage(VulkanImage&&) noexcept = default;
	VulkanImage& operator=(VulkanImage&&) noexcept = default;

	void createBuffer(uint32_t image_size);
	void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage);
	void allocateMemory();
	void createImageView(vk::Format format, vk::ImageAspectFlags aspect_flags);

	void copyBufferToImage(vk::CommandBuffer command, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
	void transitionImageLayout(vk::CommandBuffer command, vk::Image image, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout);

	vk::Image     get() const;
	vk::ImageView getView() const;
	vk::Format    getFormat() const;

	void     setSampler(VulkanSampler& sampler);
	VulkanSampler& getSampler() const;
};

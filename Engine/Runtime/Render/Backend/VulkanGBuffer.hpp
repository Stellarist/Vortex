#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"
#include "VulkanImage.hpp"
#include "VulkanSampler.hpp"

enum class VulkanGBufferAttachment {
	Position = 0,
	Normal,
	Albedo,
	Metallic,
	Roughness,
	Depth,
	Count
};

class VulkanGBuffer {
private:
	uint32_t width;
	uint32_t height;

	std::unique_ptr<VulkanSampler> sampler;

	std::unordered_map<VulkanGBufferAttachment, std::unique_ptr<VulkanImage>>                     attachments;
	std::unordered_map<VulkanGBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>> attachment_infos;

	VulkanContext* context{};

	void createAttachments();

public:
	VulkanGBuffer(VulkanContext& ctx, uint32_t width, uint32_t height,
	    std::unordered_map<VulkanGBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>> attachment_infos);
	~VulkanGBuffer() = default;

	VulkanGBuffer(const VulkanGBuffer&) = delete;
	VulkanGBuffer& operator=(const VulkanGBuffer&) = delete;

	VulkanGBuffer(VulkanGBuffer&&) noexcept = default;
	VulkanGBuffer& operator=(VulkanGBuffer&&) noexcept = default;

	void resize(uint32_t width, uint32_t height);

	VulkanImage*        getImage(VulkanGBufferAttachment attachment) const;
	vk::ImageView getImageView(VulkanGBufferAttachment attachment) const;
};

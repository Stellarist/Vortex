#include "VulkanGBuffer.hpp"

VulkanGBuffer::VulkanGBuffer(VulkanContext& ctx, uint32_t width, uint32_t height,
    std::unordered_map<VulkanGBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>> attachment_infos) :
    context(&ctx),
    width(width),
    height(height),
    attachment_infos(std::move(attachment_infos)),
    sampler(std::make_unique<VulkanSampler>(ctx))
{
	createAttachments();
}

void VulkanGBuffer::createAttachments()
{
	attachments.reserve(attachment_infos.size());
	for (const auto& [attach, infos] : attachment_infos) {
		auto attach_image = std::make_unique<VulkanImage>(*context, width, height, infos.first, infos.second);
		attach_image->setSampler(*sampler);

		attachments.emplace(attach, std::move(attach_image));
	}
}

void VulkanGBuffer::resize(uint32_t width, uint32_t height)
{
	this->width = width;
	this->height = height;

	createAttachments();
}

VulkanImage* VulkanGBuffer::getImage(VulkanGBufferAttachment attachment) const
{
	return attachments.at(attachment).get();
}

vk::ImageView VulkanGBuffer::getImageView(VulkanGBufferAttachment attachment) const
{
	return attachments.at(attachment)->getView();
}

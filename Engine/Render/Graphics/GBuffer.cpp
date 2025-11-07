#include "GBuffer.hpp"

GBuffer::GBuffer(Context& ctx, uint32_t width, uint32_t height,
    std::unordered_map<GBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>> attachment_infos) :
    context(&ctx),
    width(width),
    height(height),
    attachment_infos(std::move(attachment_infos)),
    sampler(std::make_unique<Sampler>(ctx))
{
	createAttachments();
}

void GBuffer::createAttachments()
{
	attachments.reserve(attachment_infos.size());
	for (const auto& [attach, infos] : attachment_infos) {
		auto attach_image = std::make_unique<Image>(*context, width, height, infos.first, infos.second);
		attach_image->setSampler(*sampler);

		attachments.emplace(attach, std::move(attach_image));
	}
}

void GBuffer::resize(uint32_t width, uint32_t height)
{
	this->width = width;
	this->height = height;

	createAttachments();
}

Image* GBuffer::getImage(GBufferAttachment attachment) const
{
	return attachments.at(attachment).get();
}

vk::ImageView GBuffer::getImageView(GBufferAttachment attachment) const
{
	return attachments.at(attachment)->getView();
}

module Runtime.Graphics;

import vulkan;

namespace Vortex {

static vk::AttachmentLoadOp toVkAttachmentLoadOp(RHILoadOp load_op) noexcept
{
	switch (load_op) {
	case RHILoadOp::Load:
		return vk::AttachmentLoadOp::eLoad;
	case RHILoadOp::Clear:
		return vk::AttachmentLoadOp::eClear;
	case RHILoadOp::Discard:
		return vk::AttachmentLoadOp::eDontCare;
	}
	return vk::AttachmentLoadOp::eDontCare;
}

static vk::AttachmentStoreOp toVkAttachmentStoreOp(RHIStoreOp store_op) noexcept
{
	switch (store_op) {
	case RHIStoreOp::Store:
		return vk::AttachmentStoreOp::eStore;
	case RHIStoreOp::Discard:
		return vk::AttachmentStoreOp::eDontCare;
	}
	return vk::AttachmentStoreOp::eDontCare;
}

RHIRef<RHIFramebuffer> VulkanDevice::createFramebuffer(const RHIFramebufferDesc& desc)
{
	validateRHIFramebufferDesc(desc);
	auto framebuffer = makeRHIRef<VulkanFramebuffer>(desc);

	for (const auto& attachment : desc.color_attachments) {
		auto* texture_view = static_cast<VulkanTextureView*>(attachment.texture_view.get());
		framebuffer->color_attachments_info.emplace_back()
		    .setImageView(texture_view->getHandle())
		    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
		    .setLoadOp(toVkAttachmentLoadOp(attachment.load_op))
		    .setStoreOp(toVkAttachmentStoreOp(attachment.store_op))
		    .setClearValue(vk::ClearValue{}.setColor(toVkClearColorValue(attachment.clear_value.color)));
	}

	if (desc.depth_attachment.texture_view) {
		auto* texture_view = static_cast<VulkanTextureView*>(desc.depth_attachment.texture_view.get());
		auto& texture = texture_view->getTexture();

		const auto layout = desc.depth_attachment.read_only ?
		    vk::ImageLayout::eDepthStencilReadOnlyOptimal :
		    vk::ImageLayout::eDepthStencilAttachmentOptimal;

		framebuffer->depth_attachment_info.setImageView(texture_view->getHandle())
		    .setImageLayout(layout)
		    .setLoadOp(toVkAttachmentLoadOp(desc.depth_attachment.load_op))
		    .setStoreOp(toVkAttachmentStoreOp(desc.depth_attachment.store_op))
		    .setClearValue(vk::ClearValue{}.setDepthStencil(
		        {desc.depth_attachment.clear_value.depth, desc.depth_attachment.clear_value.stencil}));

		if (getVkImageAspectFlags(texture.getDesc().format) & vk::ImageAspectFlagBits::eStencil)
			framebuffer->stencil_attachment_info.setImageView(texture_view->getHandle())
			    .setImageLayout(layout)
			    .setLoadOp(toVkAttachmentLoadOp(desc.depth_attachment.load_op))
			    .setStoreOp(toVkAttachmentStoreOp(desc.depth_attachment.store_op))
			    .setClearValue(vk::ClearValue{}.setDepthStencil(
			        {desc.depth_attachment.clear_value.depth, desc.depth_attachment.clear_value.stencil}));
	}

	return framebuffer;
}

}        // namespace Vortex

export module Runtime.Vulkan:Framebuffer;

import vulkan;
import Core;
import :Device;
import Runtime.RHI;

export namespace Vortex {

class VulkanFramebuffer : public RHIFramebuffer {
private:
	RHIFramebufferDesc desc{};
	RHIFramebufferInfo info{};

	std::vector<vk::RenderingAttachmentInfo> color_attachments_info{};
	vk::RenderingAttachmentInfo depth_attachment_info{};
	vk::RenderingAttachmentInfo stencil_attachment_info{};

	friend class VulkanDevice;

public:
	VulkanFramebuffer(RHIFramebufferDesc desc) : desc(std::move(desc)), info(this->desc) {}
	~VulkanFramebuffer() override = default;

	const RHIFramebufferDesc& getDesc() const noexcept override { return desc; }
	const RHIFramebufferInfo& getFramebufferInfo() const noexcept override { return info; }

	const std::vector<vk::RenderingAttachmentInfo>& getColorAttachmentsInfo() const noexcept { return color_attachments_info; }
	const vk::RenderingAttachmentInfo& getDepthAttachmentInfo() const noexcept { return depth_attachment_info; }
	const vk::RenderingAttachmentInfo& getStencilAttachmentInfo() const noexcept { return stencil_attachment_info; }
};

}        // namespace Vortex

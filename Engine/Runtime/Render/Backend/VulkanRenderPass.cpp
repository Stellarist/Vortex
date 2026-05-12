#include "VulkanRenderPass.hpp"

#include "VulkanDevice.hpp"

VulkanRenderPass::VulkanRenderPass(VulkanContext& c, const VulkanRenderPassConfig& config) :
    context(&c)
{
	create(config);
}

VulkanRenderPass::~VulkanRenderPass()
{
	for (auto& framebuffer : framebuffers)
		context->getDevice().logical().destroyFramebuffer(framebuffer);
	context->getDevice().logical().destroyRenderPass(render_pass);
}

void VulkanRenderPass::create(const VulkanRenderPassConfig& config)
{
	vk::RenderPassCreateInfo render_pass_info{};
	render_pass_info.setAttachments(config.attachments)
	    .setSubpasses(config.subpasses)
	    .setDependencies(config.dependencies);

	render_pass = context->getDevice().logical().createRenderPass(render_pass_info);
}

void VulkanRenderPass::createFramebuffers(std::span<const std::vector<vk::ImageView>> attachments_per_frame, vk::Extent2D extent)
{
	for (auto& framebuffer : framebuffers)
		context->getDevice().logical().destroyFramebuffer(framebuffer);

	framebuffers.resize(attachments_per_frame.size());
	for (size_t i = 0; i < attachments_per_frame.size(); i++) {
		vk::FramebufferCreateInfo create_info{};
		create_info.setRenderPass(render_pass)
		    .setAttachments(attachments_per_frame[i])
		    .setWidth(extent.width)
		    .setHeight(extent.height)
		    .setLayers(1);

		framebuffers[i] = context->getDevice().logical().createFramebuffer(create_info);
	}
}

void VulkanRenderPass::begin(vk::CommandBuffer command_buffer, uint32_t framebuffer_index,
    const vk::Extent2D& extent, std::span<const vk::ClearValue> clear_values)
{
	vk::RenderPassBeginInfo begin_info{};
	begin_info.setRenderPass(render_pass)
	    .setFramebuffer(framebuffers[framebuffer_index])
	    .setRenderArea({{0, 0}, extent})
	    .setClearValues(clear_values);

	command_buffer.beginRenderPass(begin_info, vk::SubpassContents::eInline);
}

void VulkanRenderPass::end(vk::CommandBuffer command_buffer)
{
	command_buffer.endRenderPass();
}

void VulkanRenderPass::next(vk::CommandBuffer command_buffer)
{
	command_buffer.nextSubpass(vk::SubpassContents::eInline);
}

vk::RenderPass VulkanRenderPass::get() const
{
	return render_pass;
}

uint32_t VulkanRenderPass::getFramebufferCount() const
{
	return static_cast<uint32_t>(framebuffers.size());
}

#include "RenderPass.hpp"

RenderPass::RenderPass(Context& c, SwapChain& s, const RenderPassConfig& p) :
    context(&c),
    swap_chain(&s),
    config(p)
{
	config.attachments.front().setFormat(swap_chain->getSurfaceFormat().format);

	create(config);
	createFrameBuffers(swap_chain->getImageViews(), swap_chain->getExtent(), swap_chain->getImageCount());
}

RenderPass::~RenderPass()
{
	for (auto& framebuffer : framebuffers)
		context->getLogicalDevice().destroyFramebuffer(framebuffer);
	context->getLogicalDevice().destroyRenderPass(render_pass);
}

void RenderPass::create(const RenderPassConfig& config)
{
	vk::RenderPassCreateInfo render_pass_info{};
	render_pass_info.setAttachments(config.attachments)
	    .setSubpasses(config.subpasses)
	    .setDependencies(config.dependencies);

	render_pass = context->getLogicalDevice().createRenderPass(render_pass_info);
}

void RenderPass::createFrameBuffers(std::span<const vk::ImageView> attachments, vk::Extent2D extent, uint32_t count)
{
	framebuffers.resize(count);
	for (size_t i = 0; i < count; i++) {
		vk::FramebufferCreateInfo create_info{};
		create_info.setRenderPass(render_pass)
		    .setAttachments(attachments[i])
		    .setWidth(extent.width)
		    .setHeight(extent.height)
		    .setLayers(1);

		framebuffers[i] = context->getLogicalDevice().createFramebuffer(create_info);
	}
}

void RenderPass::begin(vk::CommandBuffer command_buffer, uint32_t framebuffer_index, const vk::Extent2D& extent, const vk::ClearValue& color)
{
	vk::RenderPassBeginInfo begin_info{};
	begin_info.setRenderPass(render_pass)
	    .setFramebuffer(framebuffers[framebuffer_index])
	    .setRenderArea({{0, 0}, extent})
	    .setClearValues(color);

	command_buffer.beginRenderPass(begin_info, vk::SubpassContents::eInline);
}

void RenderPass::end(vk::CommandBuffer command_buffer)
{
	command_buffer.endRenderPass();
}

void RenderPass::next(vk::CommandBuffer command_buffer)
{
	command_buffer.nextSubpass(vk::SubpassContents::eInline);
}

vk::RenderPass RenderPass::get() const
{
	return render_pass;
}

const RenderPassConfig& RenderPass::getConfig() const
{
	return config;
}

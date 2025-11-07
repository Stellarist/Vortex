#include "ForwardPass.hpp"

#include "Render/Graphics/Device.hpp"
#include "Render/Graphics/SwapChain.hpp"

ForwardPass::ForwardPass()
{
	type = PassType::Forward;
}

ForwardPass::~ForwardPass()
{
	cleanup();
}

RenderPassConfig ForwardPass::createConfig()
{
	RenderPassConfig config;

	static vk::AttachmentReference color_ref{0, vk::ImageLayout::eColorAttachmentOptimal};
	static vk::AttachmentReference depth_ref{1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

	config.attachments.push_back(
	    vk::AttachmentDescription()
	        .setFormat(context->getSwapChain().getSurfaceFormat().format)
	        .setSamples(vk::SampleCountFlagBits::e1)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eStore)
	        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	        .setInitialLayout(vk::ImageLayout::eUndefined)
	        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR));

	config.attachments.push_back(
	    vk::AttachmentDescription()
	        .setFormat(vk::Format::eD32Sfloat)
	        .setSamples(vk::SampleCountFlagBits::e1)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
	        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	        .setInitialLayout(vk::ImageLayout::eUndefined)
	        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal));

	config.subpasses.push_back(
	    vk::SubpassDescription()
	        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
	        .setColorAttachments(color_ref)
	        .setPDepthStencilAttachment(&depth_ref));

	config.dependencies.push_back(
	    vk::SubpassDependency()
	        .setSrcSubpass(vk::SubpassExternal)
	        .setDstSubpass(0)
	        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
	        .setSrcAccessMask(vk::AccessFlagBits::eNone)
	        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
	        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite));

	return config;
}

void ForwardPass::createDepthImage()
{
	depth_image = std::make_unique<Image>(
	    *context,
	    context->getSwapChain().getExtent().width,
	    context->getSwapChain().getExtent().height,
	    vk::Format::eD32Sfloat,
	    vk::ImageUsageFlagBits::eDepthStencilAttachment);
}

void ForwardPass::createFramebuffers()
{
	auto image_views = context->getSwapChain().getImageViews();

	std::vector<std::vector<vk::ImageView>> attachments;

	for (auto view : image_views)
		attachments.push_back({view, depth_image->getView()});

	pass->createFramebuffers(attachments, extent);
}

void ForwardPass::initialize(Context& ctx, vk::Extent2D ext)
{
	context = &ctx;
	extent = ext;

	createDepthImage();
	pass = std::make_unique<RenderPass>(ctx, createConfig());
	createFramebuffers();
}

void ForwardPass::cleanup()
{
	if (context) {
		context->getDevice().logical().waitIdle();
		pass.reset();
		depth_image.reset();
	}
}

void ForwardPass::resize(vk::Extent2D new_extent)
{
	if (!context)
		return;

	extent = new_extent;

	depth_image.reset();
	createDepthImage();

	createFramebuffers();
}

Image& ForwardPass::getDepthImage() const
{
	return *depth_image;
}

#include "GeometryPass.hpp"

#include "Render/Graphics/Device.hpp"

GeometryPass::GeometryPass()
{
	type = PassType::Geometry;
}

GeometryPass::~GeometryPass()
{
	cleanup();
}

RenderPassConfig GeometryPass::createConfig()
{
	RenderPassConfig config;

	// Attachment references
	static std::vector<vk::AttachmentReference> color_refs = {
	    {0, vk::ImageLayout::eColorAttachmentOptimal},        // Position
	    {1, vk::ImageLayout::eColorAttachmentOptimal},        // Normal
	    {2, vk::ImageLayout::eColorAttachmentOptimal},        // Albedo
	    {3, vk::ImageLayout::eColorAttachmentOptimal},        // Metallic
	    {4, vk::ImageLayout::eColorAttachmentOptimal},        // Roughness
	};
	static vk::AttachmentReference depth_ref{5, vk::ImageLayout::eDepthStencilAttachmentOptimal};

	// Position attachment
	config.attachments.push_back(
	    vk::AttachmentDescription()
	        .setFormat(attachment_infos.at(GBufferAttachment::Position).first)
	        .setSamples(vk::SampleCountFlagBits::e1)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eStore)
	        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	        .setInitialLayout(vk::ImageLayout::eUndefined)
	        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal));

	// Normal attachment
	config.attachments.push_back(
	    vk::AttachmentDescription()
	        .setFormat(attachment_infos.at(GBufferAttachment::Normal).first)
	        .setSamples(vk::SampleCountFlagBits::e1)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eStore)
	        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	        .setInitialLayout(vk::ImageLayout::eUndefined)
	        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal));

	// Albedo attachment
	config.attachments.push_back(
	    vk::AttachmentDescription()
	        .setFormat(attachment_infos.at(GBufferAttachment::Albedo).first)
	        .setSamples(vk::SampleCountFlagBits::e1)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eStore)
	        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	        .setInitialLayout(vk::ImageLayout::eUndefined)
	        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal));

	// Metallic attachment
	config.attachments.push_back(
	    vk::AttachmentDescription()
	        .setFormat(attachment_infos.at(GBufferAttachment::Metallic).first)
	        .setSamples(vk::SampleCountFlagBits::e1)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eStore)
	        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	        .setInitialLayout(vk::ImageLayout::eUndefined)
	        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal));

	// Roughness attachment
	config.attachments.push_back(
	    vk::AttachmentDescription()
	        .setFormat(attachment_infos.at(GBufferAttachment::Roughness).first)
	        .setSamples(vk::SampleCountFlagBits::e1)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eStore)
	        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	        .setInitialLayout(vk::ImageLayout::eUndefined)
	        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal));

	// Depth attachment
	config.attachments.push_back(
	    vk::AttachmentDescription()
	        .setFormat(attachment_infos.at(GBufferAttachment::Depth).first)
	        .setSamples(vk::SampleCountFlagBits::e1)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eStore)
	        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	        .setInitialLayout(vk::ImageLayout::eUndefined)
	        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal));

	// Subpass
	config.subpasses.push_back(
	    vk::SubpassDescription()
	        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
	        .setColorAttachments(color_refs)
	        .setPDepthStencilAttachment(&depth_ref));

	// Dependencies
	config.dependencies.push_back(
	    vk::SubpassDependency()
	        .setSrcSubpass(vk::SubpassExternal)
	        .setDstSubpass(0)
	        .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
	        .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
	        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
	        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite));

	config.dependencies.push_back(
	    vk::SubpassDependency()
	        .setSrcSubpass(0)
	        .setDstSubpass(vk::SubpassExternal)
	        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests)
	        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
	        .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
	        .setDstAccessMask(vk::AccessFlagBits::eShaderRead));

	return config;
}

void GeometryPass::createAttachmentInfos()
{
	attachment_infos[GBufferAttachment::Position] = {
	    vk::Format::eR32G32B32A32Sfloat,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	};

	attachment_infos[GBufferAttachment::Normal] = {
	    vk::Format::eR16G16B16A16Sfloat,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	};

	attachment_infos[GBufferAttachment::Albedo] = {
	    vk::Format::eR8G8B8A8Unorm,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	};

	attachment_infos[GBufferAttachment::Metallic] = {
	    vk::Format::eR8Unorm,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	};

	attachment_infos[GBufferAttachment::Roughness] = {
	    vk::Format::eR8Unorm,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	};

	attachment_infos[GBufferAttachment::Depth] = {
	    vk::Format::eD32Sfloat,
	    vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
	};
}

void GeometryPass::createFramebuffers()
{
	if (!gbuffer)
		return;

	std::vector<vk::ImageView> gbuffer_attachments = {
	    gbuffer->getImageView(GBufferAttachment::Position),
	    gbuffer->getImageView(GBufferAttachment::Normal),
	    gbuffer->getImageView(GBufferAttachment::Albedo),
	    gbuffer->getImageView(GBufferAttachment::Metallic),
	    gbuffer->getImageView(GBufferAttachment::Roughness),
	    gbuffer->getImageView(GBufferAttachment::Depth)};

	std::vector<std::vector<vk::ImageView>> attachments_per_frame = {gbuffer_attachments};
	pass->createFramebuffers(attachments_per_frame, extent);
}

void GeometryPass::initialize(Context& ctx, vk::Extent2D ext)
{
	context = &ctx;
	extent = ext;

	createAttachmentInfos();
	pass = std::make_unique<RenderPass>(ctx, createConfig());
}

void GeometryPass::cleanup()
{
	if (context) {
		context->getDevice().logical().waitIdle();
		pass.reset();
		gbuffer = nullptr;
	}
}

void GeometryPass::resize(vk::Extent2D new_extent)
{
	if (!context)
		return;

	extent = new_extent;

	if (gbuffer)
		createFramebuffers();
}

void GeometryPass::setGBuffer(GBuffer& buffer)
{
	gbuffer = &buffer;
	createFramebuffers();
}

const std::unordered_map<GBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>>& GeometryPass::getGBufferAttachmentInfos() const
{
	return attachment_infos;
}

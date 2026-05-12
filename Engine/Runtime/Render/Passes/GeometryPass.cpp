#include "GeometryPass.hpp"

#include "Runtime/Render/Backend/VulkanDevice.hpp"

GeometryPass::GeometryPass()
{
	type = PassType::Geometry;
}

GeometryPass::~GeometryPass()
{
	cleanup();
}

VulkanRenderPassConfig GeometryPass::createConfig()
{
	VulkanRenderPassConfig config;

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
	        .setFormat(attachment_infos.at(VulkanGBufferAttachment::Position).first)
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
	        .setFormat(attachment_infos.at(VulkanGBufferAttachment::Normal).first)
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
	        .setFormat(attachment_infos.at(VulkanGBufferAttachment::Albedo).first)
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
	        .setFormat(attachment_infos.at(VulkanGBufferAttachment::Metallic).first)
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
	        .setFormat(attachment_infos.at(VulkanGBufferAttachment::Roughness).first)
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
	        .setFormat(attachment_infos.at(VulkanGBufferAttachment::Depth).first)
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
	attachment_infos[VulkanGBufferAttachment::Position] = {
	    vk::Format::eR32G32B32A32Sfloat,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	};

	attachment_infos[VulkanGBufferAttachment::Normal] = {
	    vk::Format::eR16G16B16A16Sfloat,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	};

	attachment_infos[VulkanGBufferAttachment::Albedo] = {
	    vk::Format::eR8G8B8A8Unorm,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	};

	attachment_infos[VulkanGBufferAttachment::Metallic] = {
	    vk::Format::eR8Unorm,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	};

	attachment_infos[VulkanGBufferAttachment::Roughness] = {
	    vk::Format::eR8Unorm,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	};

	attachment_infos[VulkanGBufferAttachment::Depth] = {
	    vk::Format::eD32Sfloat,
	    vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
	};
}

void GeometryPass::createFramebuffers()
{
	if (!gbuffer)
		return;

	std::vector<vk::ImageView> gbuffer_attachments = {
	    gbuffer->getImageView(VulkanGBufferAttachment::Position),
	    gbuffer->getImageView(VulkanGBufferAttachment::Normal),
	    gbuffer->getImageView(VulkanGBufferAttachment::Albedo),
	    gbuffer->getImageView(VulkanGBufferAttachment::Metallic),
	    gbuffer->getImageView(VulkanGBufferAttachment::Roughness),
	    gbuffer->getImageView(VulkanGBufferAttachment::Depth)};

	std::vector<std::vector<vk::ImageView>> attachments_per_frame = {gbuffer_attachments};
	pass->createFramebuffers(attachments_per_frame, extent);
}

void GeometryPass::initialize(VulkanContext& ctx, vk::Extent2D ext)
{
	context = &ctx;
	extent = ext;

	createAttachmentInfos();
	pass = std::make_unique<VulkanRenderPass>(ctx, createConfig());
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

void GeometryPass::setGBuffer(VulkanGBuffer& buffer)
{
	gbuffer = &buffer;
	createFramebuffers();
}

const std::unordered_map<VulkanGBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>>& GeometryPass::getGBufferAttachmentInfos() const
{
	return attachment_infos;
}

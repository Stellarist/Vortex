#include "LightingPass.hpp"

#include "Render/Graphics/Device.hpp"
#include "Render/Graphics/SwapChain.hpp"

LightingPass::LightingPass()
{
	type = PassType::Lighting;
}

LightingPass::~LightingPass()
{
	cleanup();
}

RenderPassConfig LightingPass::createConfig()
{
	RenderPassConfig config;

	static vk::AttachmentReference color_ref{0, vk::ImageLayout::eColorAttachmentOptimal};

	// Color attachment
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

	// Subpass
	config.subpasses.push_back(
	    vk::SubpassDescription()
	        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
	        .setColorAttachments(color_ref));

	// Dependency
	config.dependencies.push_back(
	    vk::SubpassDependency()
	        .setSrcSubpass(vk::SubpassExternal)
	        .setDstSubpass(0)
	        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
	        .setSrcAccessMask(vk::AccessFlagBits::eNone)
	        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
	        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite));

	return config;
}

void LightingPass::createFramebuffers()
{
	auto image_views = context->getSwapChain().getImageViews();

	std::vector<std::vector<vk::ImageView>> attachments;

	for (auto view : image_views)
		attachments.push_back({view});

	pass->createFramebuffers(attachments, extent);
}

void LightingPass::initialize(Context& ctx, vk::Extent2D ext)
{
	context = &ctx;
	extent = ext;

	pass = std::make_unique<RenderPass>(ctx, createConfig());
	createFramebuffers();
}

void LightingPass::cleanup()
{
	if (context) {
		context->getDevice().logical().waitIdle();

		gbuffer_pool.reset();
		gbuffer_layout.reset();

		pass.reset();
	}
}

void LightingPass::resize(vk::Extent2D new_extent)
{
	if (!context)
		return;

	extent = new_extent;

	createFramebuffers();
}

void LightingPass::createGBufferDescriptorSetLayout()
{
	std::vector<vk::DescriptorSetLayoutBinding> bindings;

	// Binding 0: Position
	bindings.push_back(
	    vk::DescriptorSetLayoutBinding()
	        .setBinding(0)
	        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
	        .setDescriptorCount(1)
	        .setStageFlags(vk::ShaderStageFlagBits::eFragment));

	// Binding 1: Normal
	bindings.push_back(
	    vk::DescriptorSetLayoutBinding()
	        .setBinding(1)
	        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
	        .setDescriptorCount(1)
	        .setStageFlags(vk::ShaderStageFlagBits::eFragment));

	// Binding 2: Albedo
	bindings.push_back(
	    vk::DescriptorSetLayoutBinding()
	        .setBinding(2)
	        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
	        .setDescriptorCount(1)
	        .setStageFlags(vk::ShaderStageFlagBits::eFragment));

	// Binding 3: Metallic
	bindings.push_back(
	    vk::DescriptorSetLayoutBinding()
	        .setBinding(3)
	        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
	        .setDescriptorCount(1)
	        .setStageFlags(vk::ShaderStageFlagBits::eFragment));

	// Binding 4: Roughness
	bindings.push_back(
	    vk::DescriptorSetLayoutBinding()
	        .setBinding(4)
	        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
	        .setDescriptorCount(1)
	        .setStageFlags(vk::ShaderStageFlagBits::eFragment));

	gbuffer_layout = std::make_unique<DescriptorSetLayout>(*context, bindings);
}

void LightingPass::createGBufferDescriptorSet(const GBuffer& gbuffer)
{
	if (!gbuffer_layout)
		createGBufferDescriptorSetLayout();

	std::vector<vk::DescriptorPoolSize> pool_sizes = {{vk::DescriptorType::eCombinedImageSampler, 5}};
	gbuffer_pool = std::make_unique<DescriptorPool>(*context, 1, pool_sizes);

	gbuffer_descriptor = gbuffer_pool->allocate(*gbuffer_layout);

	updateGBufferDescriptorSet(gbuffer);
}

void LightingPass::updateGBufferDescriptorSet(const GBuffer& gbuffer)
{
	auto& device = context->getDevice();

	gbuffer_descriptor.update(device, 0, vk::DescriptorType::eCombinedImageSampler,
	    gbuffer.getImage(GBufferAttachment::Position));

	gbuffer_descriptor.update(device, 1, vk::DescriptorType::eCombinedImageSampler,
	    gbuffer.getImage(GBufferAttachment::Normal));

	gbuffer_descriptor.update(device, 2, vk::DescriptorType::eCombinedImageSampler,
	    gbuffer.getImage(GBufferAttachment::Albedo));

	gbuffer_descriptor.update(device, 3, vk::DescriptorType::eCombinedImageSampler,
	    gbuffer.getImage(GBufferAttachment::Metallic));

	gbuffer_descriptor.update(device, 4, vk::DescriptorType::eCombinedImageSampler,
	    gbuffer.getImage(GBufferAttachment::Roughness));
}

void LightingPass::setupGBuffer(const GBuffer& gbuffer)
{
	createGBufferDescriptorSet(gbuffer);
}

void LightingPass::bindGBufferDescriptor(vk::CommandBuffer command, vk::PipelineLayout pipeline_layout) const
{
	command.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics,
	    pipeline_layout,
	    3,
	    gbuffer_descriptor.get(),
	    {});
}

const DescriptorSetLayout& LightingPass::getGBufferLayout() const
{
	return *gbuffer_layout;
}

#include "DeferredPath.hpp"

#include "Render/Graphics/Device.hpp"
#include "Render/Graphics/SwapChain.hpp"
#include "Render/RHI/GpuData.hpp"

std::vector<vk::PipelineColorBlendAttachmentState> DeferredPath::color_blend_attachments = {};

DeferredPath::DeferredPath()
{
	type = PathType::Deferred;
}

DeferredPath::~DeferredPath()
{
	cleanup();
}

void DeferredPath::initialize(Context& ctx)
{
	context = &ctx;

	auto extent = context->getSwapChain().getExtent();

	geometry_pass = std::make_unique<GeometryPass>();
	geometry_pass->initialize(ctx, extent);

	lighting_pass = std::make_unique<LightingPass>();
	lighting_pass->initialize(ctx, extent);

	auto& attachment_infos = geometry_pass->getGBufferAttachmentInfos();
	gbuffer = std::make_unique<GBuffer>(ctx, extent.width, extent.height, attachment_infos);
	geometry_pass->setGBuffer(*gbuffer);
}

void DeferredPath::cleanup()
{
	if (context) {
		context->getDevice().logical().waitIdle();

		gbuffer.reset();
		geometry_pipeline.reset();
		lighting_pipeline.reset();
		lighting_pass.reset();
		geometry_pass.reset();
	}
}

void DeferredPath::resize(uint32_t width, uint32_t height)
{
	if (!context)
		return;

	vk::Extent2D extent{width, height};

	if (gbuffer)
		gbuffer->resize(extent.width, extent.height);

	if (geometry_pass)
		geometry_pass->resize(extent);

	if (lighting_pass) {
		lighting_pass->resize(extent);

		if (gbuffer)
			lighting_pass->updateGBufferDescriptorSet(*gbuffer);
	}
}

GraphicsPipelineConfig DeferredPath::createGeometryPipelineConfig()
{
	GraphicsPipelineConfig config{};

	// Color blend attachments
	color_blend_attachments.resize(static_cast<size_t>(GBufferAttachment::Count) - 1);
	for (auto& attachment : color_blend_attachments)
		attachment.setBlendEnable(vk::False)
		    .setColorWriteMask(vk::ColorComponentFlagBits::eR
		        | vk::ColorComponentFlagBits::eG
		        | vk::ColorComponentFlagBits::eB
		        | vk::ColorComponentFlagBits::eA);

	// Vertex input
	config.vertex_binding = GpuVertex::binding();
	config.vertex_attributes = GpuVertex::attributes();
	config.vertex_input
	    .setVertexBindingDescriptions(config.vertex_binding)
	    .setVertexAttributeDescriptions(config.vertex_attributes);

	// Color blend state
	config.color_blend_state
	    .setLogicOpEnable(vk::False)
	    .setLogicOp(vk::LogicOp::eCopy)
	    .setAttachments(color_blend_attachments);

	// Depth stencil
	config.depth_stencil =
	    vk::PipelineDepthStencilStateCreateInfo()
	        .setDepthTestEnable(vk::True)
	        .setDepthWriteEnable(vk::True)
	        .setDepthCompareOp(vk::CompareOp::eLess)
	        .setDepthBoundsTestEnable(vk::False)
	        .setStencilTestEnable(vk::False)
	        .setMinDepthBounds(0.0f)
	        .setMaxDepthBounds(1.0f);

	return config;
}

GraphicsPipelineConfig DeferredPath::createLightingPipelineConfig()
{
	GraphicsPipelineConfig config{};

	// Depth stencil
	config.depth_stencil
	    .setDepthTestEnable(vk::False)
	    .setDepthWriteEnable(vk::False)
	    .setDepthBoundsTestEnable(vk::False)
	    .setStencilTestEnable(vk::False);

	// Color blend attachment
	config.color_blend_attachment
	    .setBlendEnable(vk::False)
	    .setColorWriteMask(vk::ColorComponentFlagBits::eR
	        | vk::ColorComponentFlagBits::eG
	        | vk::ColorComponentFlagBits::eB
	        | vk::ColorComponentFlagBits::eA);

	// Single color attachment
	config.color_blend_state
	    .setLogicOpEnable(vk::False)
	    .setLogicOp(vk::LogicOp::eCopy)
	    .setAttachments(config.color_blend_attachment);

	return config;
}

DeferredPath& DeferredPath::build(std::span<const vk::DescriptorSetLayout> geometry_layouts, std::span<const vk::PipelineShaderStageCreateInfo> geometry_stages,
    std::span<const vk::DescriptorSetLayout> lighting_layouts, std::span<const vk::PipelineShaderStageCreateInfo> lighting_stages)
{
	if (!geometry_pass || !lighting_pass || !gbuffer)
		return *this;

	lighting_pass->setupGBuffer(*gbuffer);

	auto geometry_config = createGeometryPipelineConfig();
	geometry_config.descriptor_layouts = {geometry_layouts.begin(), geometry_layouts.end()};
	geometry_config.pipeline_layout.setSetLayouts(geometry_config.descriptor_layouts);
	geometry_config.shader_stages = {geometry_stages.begin(), geometry_stages.end()};
	geometry_pipeline = std::make_unique<GraphicsPipeline>(*context, geometry_pass->getPass(), std::move(geometry_config));

	auto lighting_config = createLightingPipelineConfig();
	lighting_config.descriptor_layouts = {lighting_layouts.begin(), lighting_layouts.end()};
	lighting_config.descriptor_layouts.push_back(lighting_pass->getGBufferLayout().get());
	lighting_config.pipeline_layout.setSetLayouts(lighting_config.descriptor_layouts);
	lighting_config.shader_stages = {lighting_stages.begin(), lighting_stages.end()};
	lighting_pipeline = std::make_unique<GraphicsPipeline>(*context, lighting_pass->getPass(), std::move(lighting_config));

	return *this;
}

GeometryPass& DeferredPath::getGeometryPass() const
{
	return *geometry_pass;
}

LightingPass& DeferredPath::getLightingPass() const
{
	return *lighting_pass;
}

GraphicsPipeline& DeferredPath::getGeometryPipeline() const
{
	return *geometry_pipeline;
}

GraphicsPipeline& DeferredPath::getLightingPipeline() const
{
	return *lighting_pipeline;
}

GBuffer& DeferredPath::getGBuffer() const
{
	return *gbuffer;
}

void DeferredPath::beginGeometryPass(vk::CommandBuffer command, vk::Extent2D extent)
{
	std::array<vk::ClearValue, 6> clear_values{
	    vk::ClearValue{}.setColor({0.0f, 0.0f, 0.0f, 0.0f}),
	    vk::ClearValue{}.setColor({0.0f, 0.0f, 0.0f, 0.0f}),
	    vk::ClearValue{}.setColor({0.0f, 0.0f, 0.0f, 0.0f}),
	    vk::ClearValue{}.setColor({0.0f, 0.0f, 0.0f, 0.0f}),
	    vk::ClearValue{}.setColor({0.0f, 0.0f, 0.0f, 0.0f}),
	    vk::ClearValue{}.setDepthStencil({1.0f, 0}),
	};

	geometry_pass->getPass().begin(command, 0, extent, clear_values);

	command.setScissor(0, vk::Rect2D{}.setOffset({0, 0}).setExtent(extent));

	command.setViewport(0, vk::Viewport{}.setX(0.0f).setY(0.0f).setWidth(static_cast<float>(extent.width)).setHeight(static_cast<float>(extent.height)).setMinDepth(0.0f).setMaxDepth(1.0f));
}

void DeferredPath::endGeometryPass(vk::CommandBuffer command)
{
	geometry_pass->getPass().end(command);
}

void DeferredPath::beginLightingPass(vk::CommandBuffer command, uint32_t image_index, vk::Extent2D extent)
{
	std::array<vk::ClearValue, 1> clear_values{
	    vk::ClearValue{}.setColor({0.0f, 0.0f, 0.0f, 1.0f}),
	};

	lighting_pass->getPass().begin(command, image_index, extent, clear_values);

	command.setScissor(0, vk::Rect2D{}.setOffset({0, 0}).setExtent(extent));

	command.setViewport(0, vk::Viewport{}.setX(0.0f).setY(0.0f).setWidth(static_cast<float>(extent.width)).setHeight(static_cast<float>(extent.height)).setMinDepth(0.0f).setMaxDepth(1.0f));
}

void DeferredPath::endLightingPass(vk::CommandBuffer command)
{
	lighting_pass->getPass().end(command);
}

void DeferredPath::bindDescriptor(vk::CommandBuffer command) const
{
	lighting_pass->bindGBufferDescriptor(command, getLightingPipeline().getLayout());
}

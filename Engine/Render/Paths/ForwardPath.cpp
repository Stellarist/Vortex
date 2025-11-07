#include "ForwardPath.hpp"

#include "Render/Graphics/Device.hpp"
#include "Render/Graphics/SwapChain.hpp"
#include "Render/RHI/GpuData.hpp"

ForwardPath::ForwardPath()
{
	type = PathType::Forward;
}

ForwardPath::~ForwardPath()
{
	cleanup();
}

void ForwardPath::initialize(Context& ctx)
{
	context = &ctx;

	auto extent = context->getSwapChain().getExtent();
	forward_pass = std::make_unique<ForwardPass>();
	forward_pass->initialize(ctx, extent);
}

void ForwardPath::cleanup()
{
	if (context) {
		context->getDevice().logical().waitIdle();

		forward_pipeline.reset();
		forward_pass.reset();
	}
}

void ForwardPath::resize(uint32_t width, uint32_t height)
{
	if (forward_pass)
		forward_pass->resize(vk::Extent2D{width, height});
}

GraphicsPipelineConfig ForwardPath::createPipelineConfig()
{
	GraphicsPipelineConfig config{};

	// Vertex input
	config.vertex_binding = GpuVertex::binding();
	config.vertex_attributes = GpuVertex::attributes();
	config.vertex_input
	    .setVertexBindingDescriptions(config.vertex_binding)
	    .setVertexAttributeDescriptions(config.vertex_attributes);

	// Depth stencil state
	config.depth_stencil
	    .setDepthTestEnable(vk::True)
	    .setDepthWriteEnable(vk::True)
	    .setDepthCompareOp(vk::CompareOp::eLess)
	    .setDepthBoundsTestEnable(vk::False)
	    .setStencilTestEnable(vk::False)
	    .setMinDepthBounds(0.0f)
	    .setMaxDepthBounds(1.0f);

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

ForwardPath& ForwardPath::build(std::span<const vk::DescriptorSetLayout> forward_layouts, std::span<const vk::PipelineShaderStageCreateInfo> forward_stages)
{
	if (!forward_pass)
		return *this;

	auto pipeline_config = createPipelineConfig();
	pipeline_config.descriptor_layouts = {forward_layouts.begin(), forward_layouts.end()};
	pipeline_config.pipeline_layout.setSetLayouts(pipeline_config.descriptor_layouts);
	pipeline_config.shader_stages = {forward_stages.begin(), forward_stages.end()};
	forward_pipeline = std::make_unique<GraphicsPipeline>(*context, forward_pass->getPass(), std::move(pipeline_config));

	return *this;
}

ForwardPass& ForwardPath::getForwardPass() const
{
	return *forward_pass;
}

GraphicsPipeline& ForwardPath::getForwardPipeline() const
{
	return *forward_pipeline;
}

void ForwardPath::beginForwardPass(vk::CommandBuffer command, uint32_t image_index, vk::Extent2D extent)
{
	std::array<vk::ClearValue, 2> clear_values{
	    vk::ClearValue{}.setColor({0.0f, 0.0f, 0.0f, 1.0f}),
	    vk::ClearValue{}.setDepthStencil({1.0f, 0}),
	};

	forward_pass->getPass().begin(command, image_index, extent, clear_values);

	command.setScissor(0,
	    vk::Rect2D{}
	        .setOffset({0, 0})
	        .setExtent(extent));

	command.setViewport(0,
	    vk::Viewport{}
	        .setX(0.0f)
	        .setY(0.0f)
	        .setWidth(static_cast<float>(extent.width))
	        .setHeight(static_cast<float>(extent.height))
	        .setMinDepth(0.0f)
	        .setMaxDepth(1.0f));
}

void ForwardPath::endForwardPass(vk::CommandBuffer command)
{
	forward_pass->getPass().end(command);
}

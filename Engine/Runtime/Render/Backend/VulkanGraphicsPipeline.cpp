#include "VulkanGraphicsPipeline.hpp"

#include "VulkanDevice.hpp"

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanContext& context, VulkanRenderPass& render_pass, VulkanGraphicsPipelineConfig pipeline_config) :
    context(&context), render_pass(&render_pass), config(std::move(pipeline_config))
{
	createLayout();
	create();
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
	context->getDevice().logical().destroyPipeline(pipeline);
	context->getDevice().logical().destroyPipelineLayout(pipeline_layout);
}

void VulkanGraphicsPipeline::createLayout()
{
	pipeline_layout = context->getDevice().logical().createPipelineLayout(config.pipeline_layout);
}

void VulkanGraphicsPipeline::create()
{
	vk::GraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.setStages(config.shader_stages)
	    .setPVertexInputState(&config.vertex_input)
	    .setPInputAssemblyState(&config.input_assembly)
	    .setPViewportState(&config.viewport)
	    .setPRasterizationState(&config.rasterizer)
	    .setPMultisampleState(&config.multisample)
	    .setPDepthStencilState(&config.depth_stencil)
	    .setPColorBlendState(&config.color_blend_state)
	    .setPDynamicState(&config.dynamic_state)
	    .setLayout(pipeline_layout)
	    .setRenderPass(render_pass->get())
	    .setSubpass(0);

	pipeline = context->getDevice().logical().createGraphicsPipeline({}, pipeline_info).value;
}

vk::Pipeline VulkanGraphicsPipeline::get() const
{
	return pipeline;
}

vk::PipelineLayout VulkanGraphicsPipeline::getLayout() const
{
	return pipeline_layout;
}

const VulkanGraphicsPipelineConfig& VulkanGraphicsPipeline::getConfig() const
{
	return config;
}

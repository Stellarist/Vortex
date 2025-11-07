#pragma once

#include <array>

#include <vulkan/vulkan.hpp>

#include "Context.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"

struct GraphicsPipelineConfig {
	vk::PipelineVertexInputStateCreateInfo vertex_input{};

	vk::PipelineInputAssemblyStateCreateInfo input_assembly{
	    {},
	    vk::PrimitiveTopology::eTriangleList,
	    vk::False,
	};

	vk::PipelineTessellationStateCreateInfo tessellation{};

	vk::PipelineViewportStateCreateInfo viewport{
	    {},
	    1,
	    {},
	    1,
	    {},
	};

	vk::PipelineRasterizationStateCreateInfo rasterizer{
	    {},
	    vk::False,
	    vk::False,
	    vk::PolygonMode::eFill,
	    vk::CullModeFlagBits::eBack,
	    vk::FrontFace::eCounterClockwise,
	    vk::False,
	    {},
	    {},
	    {},
	    1.0f,
	};

	vk::PipelineMultisampleStateCreateInfo multisample{
	    {},
	    vk::SampleCountFlagBits::e1,
	    vk::False,
	};

	vk::PipelineDepthStencilStateCreateInfo depth_stencil{};

	vk::PipelineColorBlendAttachmentState color_blend_attachment{};

	vk::PipelineColorBlendStateCreateInfo color_blend_state{};

	std::array<vk::DynamicState, 2> dynamic_states = {
	    vk::DynamicState::eViewport,
	    vk::DynamicState::eScissor};

	vk::PipelineDynamicStateCreateInfo dynamic_state{
	    {},
	    static_cast<uint32_t>(dynamic_states.size()),
	    dynamic_states.data(),
	};

	vk::PipelineLayoutCreateInfo pipeline_layout{};

	vk::VertexInputBindingDescription                vertex_binding{};
	std::vector<vk::VertexInputAttributeDescription> vertex_attributes{};

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{};

	std::vector<vk::DescriptorSetLayout> descriptor_layouts{};
};

class GraphicsPipeline {
private:
	vk::Pipeline       pipeline;
	vk::PipelineLayout pipeline_layout;

	GraphicsPipelineConfig config;

	Context*    context{};
	RenderPass* render_pass{};

public:
	GraphicsPipeline(Context& context, RenderPass& render_pass, GraphicsPipelineConfig pipeline_config);
	~GraphicsPipeline();

	GraphicsPipeline(const GraphicsPipeline&) = delete;
	GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

	GraphicsPipeline(GraphicsPipeline&&) noexcept = default;
	GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept = default;

	void createLayout();
	void create();

	vk::Pipeline       get() const;
	vk::PipelineLayout getLayout() const;

	const GraphicsPipelineConfig& getConfig() const;
	const Shader&                 getShader() const;
};

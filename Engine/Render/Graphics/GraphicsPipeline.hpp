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

	vk::PipelineColorBlendAttachmentState color_blend_attachment{
	    vk::False,
	    {},
	    {},
	    {},
	    {},
	    {},
	    {},
	    vk::ColorComponentFlagBits::eR |
	        vk::ColorComponentFlagBits::eG |
	        vk::ColorComponentFlagBits::eB |
	        vk::ColorComponentFlagBits::eA,
	};

	vk::PipelineColorBlendStateCreateInfo color_blend_state{
	    {},
	    vk::False,
	    vk::LogicOp::eCopy,
	    1,
	    &color_blend_attachment,
	};

	std::array<vk::DynamicState, 2> dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

	vk::PipelineDynamicStateCreateInfo dynamic_state{
	    {},
	    static_cast<uint32_t>(dynamic_states.size()),
	    dynamic_states.data(),
	};

	vk::PipelineLayoutCreateInfo pipeline_layout{};
};

class GraphicsPipeline {
private:
	vk::Pipeline       pipeline;
	vk::PipelineLayout pipeline_layout;

	GraphicsPipelineConfig config;

	Shader shader;

	std::vector<vk::DescriptorSetLayoutBinding> descriptor_bindings;

	Context*    context{};
	RenderPass* render_pass{};

public:
	GraphicsPipeline(Context&                                 context,
	                 RenderPass&                              render_pass,
	                 std::span<const vk::DescriptorSetLayout> descriptor_layouts,
	                 const GraphicsPipelineConfig&            pipeline_config = {});
	~GraphicsPipeline();

	GraphicsPipeline(const GraphicsPipeline&) = delete;
	GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

	GraphicsPipeline(GraphicsPipeline&&) noexcept = default;
	GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept = default;

	void create(const GraphicsPipelineConfig& config);

	vk::Pipeline       get() const;
	vk::PipelineLayout getLayout() const;

	const std::vector<vk::DescriptorSetLayoutBinding>& getDescriptorBindings() const;

	const GraphicsPipelineConfig& getConfig() const;
	const Shader&                 getShader() const;
};

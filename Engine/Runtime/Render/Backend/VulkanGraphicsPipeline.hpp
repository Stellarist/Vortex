#pragma once

#include <array>

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanShader.hpp"

struct VulkanGraphicsPipelineConfig {
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

class VulkanGraphicsPipeline {
private:
	vk::Pipeline       pipeline;
	vk::PipelineLayout pipeline_layout;

	VulkanGraphicsPipelineConfig config;

	VulkanContext*    context{};
	VulkanRenderPass* render_pass{};

public:
	VulkanGraphicsPipeline(VulkanContext& context, VulkanRenderPass& render_pass, VulkanGraphicsPipelineConfig pipeline_config);
	~VulkanGraphicsPipeline();

	VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
	VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;

	VulkanGraphicsPipeline(VulkanGraphicsPipeline&&) noexcept = default;
	VulkanGraphicsPipeline& operator=(VulkanGraphicsPipeline&&) noexcept = default;

	void createLayout();
	void create();

	vk::Pipeline       get() const;
	vk::PipelineLayout getLayout() const;

	const VulkanGraphicsPipelineConfig& getConfig() const;
	const VulkanShader&                 getShader() const;
};

module Runtime.Graphics;

import vulkan;

namespace Vortex {

RHIRef<RHIInputLayout> VulkanDevice::createInputLayout(const RHIInputLayoutDesc& desc)
{
	validateRHIInputLayoutDesc(desc);
	auto layout = makeRHIRef<VulkanInputLayout>(desc);

	for (const auto& binding_desc : desc.binding_descs) {
		vk::VertexInputBindingDescription binding{};
		binding.setBinding(binding_desc.binding)
		    .setStride(binding_desc.stride)
		    .setInputRate(binding_desc.instance ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex);
		layout->bindings.push_back(binding);
	}

	layout->attributes.reserve(desc.attribute_descs.size());
	for (const auto& attribute_desc : desc.attribute_descs) {
		vk::VertexInputAttributeDescription attribute{};
		attribute.setLocation(attribute_desc.location)
		    .setBinding(attribute_desc.binding)
		    .setFormat(toVkFormat(attribute_desc.format))
		    .setOffset(attribute_desc.offset);
		layout->attributes.push_back(attribute);
	}

	return layout;
}

RHIRef<RHIGraphicsPipeline> VulkanDevice::createGraphicsPipeline(const RHIGraphicsPipelineDesc& desc)
{
	validateRHIGraphicsPipelineDesc(desc);
	auto pipeline = makeRHIRef<VulkanGraphicsPipeline>(*this, desc);

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

	const auto& raster_state = desc.raster_state;
	const auto& depth_state = desc.depth_state;
	const auto& framebuffer_info = desc.framebuffer_info;

	auto add_shader = [&shader_stages](RHIShader* shader) {
		const auto& vk_shader = static_cast<const VulkanShader&>(*shader);
		shader_stages.emplace_back(vk::PipelineShaderStageCreateFlags{},
		    vk_shader.getStage(),
		    vk_shader.getHandle(),
		    vk_shader.getDesc().entry_point.c_str());
	};

	add_shader(desc.vertex_shader.get());
	add_shader(desc.pixel_shader.get());

	vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
	if (auto* input_layout = static_cast<const VulkanInputLayout*>(desc.input_layout.get()))
		vertex_input_info.setVertexBindingDescriptions(input_layout->getBindings())
		    .setVertexAttributeDescriptions(input_layout->getAttributes());

	vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
	input_assembly_info.setTopology(toVkPrimitiveTopology(desc.primitive_type));

	vk::PipelineViewportStateCreateInfo viewport_info{};
	viewport_info.setViewportCount(1).setScissorCount(1);

	vk::PipelineRasterizationStateCreateInfo rasterizer_info{};
	rasterizer_info.setDepthClampEnable(raster_state.depth_clamp_enable)
	    .setRasterizerDiscardEnable(raster_state.rasterizer_discard_enable)
	    .setPolygonMode(toVkPolygonMode(raster_state.polygon_mode))
	    .setCullMode(toVkCullMode(raster_state.cull_mode))
	    .setFrontFace(toVkFrontFace(raster_state.front_face))
	    .setDepthBiasEnable(raster_state.depth_bias != 0 || raster_state.depth_bias_clamp != 0.0f || raster_state.depth_bias_slope_factor != 0.0f)
	    .setDepthBiasConstantFactor(static_cast<float>(raster_state.depth_bias))
	    .setDepthBiasClamp(raster_state.depth_bias_clamp)
	    .setDepthBiasSlopeFactor(raster_state.depth_bias_slope_factor)
	    .setLineWidth(1.0f);

	vk::PipelineMultisampleStateCreateInfo multisample_info{};
	multisample_info.setRasterizationSamples(toVkSampleCountFlagBits(framebuffer_info.sample_count))
	    .setAlphaToCoverageEnable(desc.blend_state.alpha_to_coverage_enable);

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_info{};
	depth_stencil_info.setDepthTestEnable(depth_state.depth_test_enable)
	    .setDepthWriteEnable(depth_state.depth_write_enable)
	    .setDepthCompareOp(toVkCompareOp(depth_state.depth_compare_op))
	    .setDepthBoundsTestEnable(depth_state.depth_bounds_test_enable)
	    .setStencilTestEnable(depth_state.stencil_test_enable)
	    .setFront(vk::StencilOpState()
	            .setFailOp(toVkStencilOp(depth_state.front_face_stencil.fail_op))
	            .setPassOp(toVkStencilOp(depth_state.front_face_stencil.pass_op))
	            .setDepthFailOp(toVkStencilOp(depth_state.front_face_stencil.depth_fail_op))
	            .setCompareOp(toVkCompareOp(depth_state.front_face_stencil.stencil_func)))
	    .setBack(vk::StencilOpState()
	            .setFailOp(toVkStencilOp(depth_state.back_face_stencil.fail_op))
	            .setPassOp(toVkStencilOp(depth_state.back_face_stencil.pass_op))
	            .setDepthFailOp(toVkStencilOp(depth_state.back_face_stencil.depth_fail_op))
	            .setCompareOp(toVkCompareOp(depth_state.back_face_stencil.stencil_func)));

	std::vector<vk::PipelineColorBlendAttachmentState> color_blend_states;
	color_blend_states.reserve(framebuffer_info.color_formats.size());
	for (size_t index = 0; index < framebuffer_info.color_formats.size(); ++index) {
		const auto& blend = index < desc.blend_state.blend_descs.size() ?
		    desc.blend_state.blend_descs[index] :
		    RHIColorBlendState::BlendDesc{};
		color_blend_states.emplace_back()
		    .setBlendEnable(blend.blend_enable)
		    .setSrcColorBlendFactor(toVkBlendFactor(blend.src_blend))
		    .setDstColorBlendFactor(toVkBlendFactor(blend.dst_blend))
		    .setColorBlendOp(toVkBlendOp(blend.color_blend_op))
		    .setSrcAlphaBlendFactor(toVkBlendFactor(blend.src_blend_alpha))
		    .setDstAlphaBlendFactor(toVkBlendFactor(blend.dst_blend_alpha))
		    .setAlphaBlendOp(toVkBlendOp(blend.alpha_blend_op))
		    .setColorWriteMask(toVkColorComponentFlags(blend.color_write_mask));
	}
	vk::PipelineColorBlendStateCreateInfo color_blend_info{};
	color_blend_info.setAttachments(color_blend_states);

	const std::array dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamic_info{};
	dynamic_info.setDynamicStates(dynamic_states);

	std::vector<vk::DescriptorSetLayout> set_layouts;
	set_layouts.reserve(desc.binding_layouts.size());
	for (const auto& layout : desc.binding_layouts)
		set_layouts.push_back(static_cast<VulkanBindingLayout*>(layout.get())->getHandle());

	bool has_push_constants{};
	vk::PushConstantRange push_constant_range{};
	for (const auto& binding_layout : desc.binding_layouts) {
		for (const auto& item : binding_layout->getDesc().bindings) {
			if (item.type != RHIBindingType::PushConstants)
				continue;

			pipeline->push_constant_visibility = binding_layout->getDesc().visibility;
			push_constant_range = {toVkShaderStageFlags(binding_layout->getDesc().visibility), 0, item.size};
			has_push_constants = true;
		}
	}

	vk::PipelineLayoutCreateInfo layout_info{};
	layout_info.setSetLayouts(set_layouts)
	    .setPushConstantRangeCount(has_push_constants ? 1u : 0u)
	    .setPPushConstantRanges(has_push_constants ? &push_constant_range : nullptr);
	pipeline->layout = device.createPipelineLayout(layout_info);

	std::vector<vk::Format> color_formats;
	color_formats.reserve(framebuffer_info.color_formats.size());
	for (const auto format : framebuffer_info.color_formats)
		color_formats.push_back(toVkFormat(format));
	const auto depth_format = framebuffer_info.depth_format;

	vk::PipelineRenderingCreateInfo rendering_info{};
	rendering_info.setColorAttachmentFormats(color_formats)
	    .setDepthAttachmentFormat(toVkFormat(depth_format))
	    .setStencilAttachmentFormat(getVkImageAspectFlags(depth_format) & vk::ImageAspectFlagBits::eStencil ?
	            toVkFormat(depth_format) :
	            vk::Format::eUndefined);

	vk::GraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.setPNext(&rendering_info)
	    .setStages(shader_stages)
	    .setPVertexInputState(&vertex_input_info)
	    .setPInputAssemblyState(&input_assembly_info)
	    .setPViewportState(&viewport_info)
	    .setPRasterizationState(&rasterizer_info)
	    .setPMultisampleState(&multisample_info)
	    .setPDepthStencilState(&depth_stencil_info)
	    .setPColorBlendState(&color_blend_info)
	    .setPDynamicState(&dynamic_info)
	    .setLayout(pipeline->layout);

	pipeline->pipeline = device.createGraphicsPipeline({}, pipeline_info).value;
	return pipeline;
}

RHIRef<RHIComputePipeline> VulkanDevice::createComputePipeline(const RHIComputePipelineDesc& desc)
{
	validateRHIComputePipelineDesc(desc);
	const auto queue_family = context->getQueueIndices().graphics_family.value();
	const auto queue_properties = context->getPhysicalDevice().getQueueFamilyProperties();
	CHECK(queue_family < queue_properties.size() &&
	        queue_properties[queue_family].queueFlags & vk::QueueFlagBits::eCompute,
	    "The selected Vulkan device does not support compute pipelines");

	auto pipeline = makeRHIRef<VulkanComputePipeline>(*this, desc);

	std::vector<vk::DescriptorSetLayout> set_layouts;
	set_layouts.reserve(desc.binding_layouts.size());
	for (const auto& layout : desc.binding_layouts)
		set_layouts.push_back(static_cast<VulkanBindingLayout*>(layout.get())->getHandle());

	bool has_push_constants{};
	vk::PushConstantRange push_constant_range{};
	for (const auto& binding_layout : desc.binding_layouts) {
		for (const auto& item : binding_layout->getDesc().bindings) {
			if (item.type != RHIBindingType::PushConstants)
				continue;

			pipeline->push_constant_visibility = binding_layout->getDesc().visibility;
			push_constant_range = {toVkShaderStageFlags(binding_layout->getDesc().visibility), 0, item.size};
			has_push_constants = true;
		}
	}

	vk::PipelineLayoutCreateInfo layout_info{};
	layout_info.setSetLayouts(set_layouts)
	    .setPushConstantRangeCount(has_push_constants ? 1u : 0u)
	    .setPPushConstantRanges(has_push_constants ? &push_constant_range : nullptr);
	pipeline->layout = device.createPipelineLayout(layout_info);

	const auto& shader = static_cast<const VulkanShader&>(*desc.compute_shader);
	vk::PipelineShaderStageCreateInfo stage_info{};
	stage_info.setStage(shader.getStage())
	    .setModule(shader.getHandle())
	    .setPName(shader.getDesc().entry_point.c_str());

	vk::ComputePipelineCreateInfo pipeline_info{};
	pipeline_info.setStage(stage_info).setLayout(pipeline->layout);
	pipeline->pipeline = device.createComputePipeline({}, pipeline_info).value;
	return pipeline;
}

void VulkanDevice::destroyGraphicsPipeline(VulkanGraphicsPipeline* pipeline) noexcept
{
	if (pipeline->pipeline)
		device.destroyPipeline(pipeline->pipeline);
	if (pipeline->layout)
		device.destroyPipelineLayout(pipeline->layout);
	pipeline->pipeline = vk::Pipeline{};
	pipeline->layout = vk::PipelineLayout{};
}

void VulkanDevice::destroyComputePipeline(VulkanComputePipeline* pipeline) noexcept
{
	if (pipeline->pipeline)
		device.destroyPipeline(pipeline->pipeline);
	if (pipeline->layout)
		device.destroyPipelineLayout(pipeline->layout);
	pipeline->pipeline = vk::Pipeline{};
	pipeline->layout = vk::PipelineLayout{};
}

}        // namespace Vortex

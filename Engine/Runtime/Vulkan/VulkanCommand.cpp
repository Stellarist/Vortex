module Runtime.Vulkan;

import vulkan;

namespace Vortex {

RHIRef<RHICommandList> VulkanDevice::createCommandList(const RHICommandListDesc& desc)
{
	validateRHICommandListDesc(desc);

	return makeRHIRef<VulkanCommandList>(*this, desc);
}

void VulkanDevice::executeCommandList(RHICommandList* command_list)
{
	auto* vk_command_list = dynamic_cast<VulkanCommandList*>(command_list);
	CHECK(vk_command_list, "Vulkan execution requires a Vulkan command list");
	queue->submit(vk_command_list);
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& device, VulkanCommandPool& pool) :
    device(device), pool(pool)
{
	vk::CommandBufferAllocateInfo buffer_info{};
	buffer_info.setCommandPool(pool.getHandle())
	    .setLevel(vk::CommandBufferLevel::ePrimary)
	    .setCommandBufferCount(1);

	buffer = device.getHandle().allocateCommandBuffers(buffer_info).front();
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
	device.getHandle().freeCommandBuffers(pool.getHandle(), buffer);
}

void VulkanCommandBuffer::beginRendering(const vk::RenderingInfo& render_info)
{
	buffer.beginRendering(render_info);
}

void VulkanCommandBuffer::endRendering()
{
	buffer.endRendering();
}

void VulkanCommandBuffer::trackResource(RHIResource* resource)
{
	if (!resource)
		return;

	const auto tracked = std::find_if(tracked_resources.begin(),
	    tracked_resources.end(),
	    [resource](const RHIRef<RHIResource>& candidate) { return candidate.get() == resource; });

	if (tracked == tracked_resources.end())
		tracked_resources.emplace_back(resource);
}

void VulkanCommandBuffer::resetResources() noexcept
{
	tracked_resources.clear();
}


VulkanCommandPool::VulkanCommandPool(VulkanDevice& device, VulkanQueue& queue) :
    device(device), queue(queue)
{
	vk::CommandPoolCreateInfo pool_info{};
	pool_info.setQueueFamilyIndex(queue.getFamilyIndex())
	    .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	pool = device.getHandle().createCommandPool(pool_info);
}

VulkanCommandPool::~VulkanCommandPool()
{
	cmd_buffers.clear();

	device.getHandle().destroyCommandPool(pool);
	pool = nullptr;
}

VulkanCommandBuffer* VulkanCommandPool::createCommandBuffer()
{
	for (const auto& command : cmd_buffers) {
		if (!command->available)
			continue;

		command->getHandle().reset();
		command->available = false;
		return command.get();
	}

	auto command = std::make_unique<VulkanCommandBuffer>(device, *this);
	auto* result = command.get();
	cmd_buffers.push_back(std::move(command));
	return result;
}

void VulkanCommandPool::releaseCommandBuffer(VulkanCommandBuffer* cmd_buffer) noexcept
{
	cmd_buffer->resetResources();
	cmd_buffer->available = true;
}


void VulkanCommandList::bindSets(const std::vector<RHIRef<RHIBindingSet>>& sets,
    const std::vector<RHIRef<RHIBindingLayout>>& layouts, vk::ShaderStageFlags stages)
{
	validateRHIBindingSets(sets, layouts);

	std::vector<vk::DescriptorSet> descriptor_sets;
	descriptor_sets.reserve(sets.size());
	for (const auto& set : sets) {
		if (auto* table = dynamic_cast<VulkanDescriptorTable*>(set.get())) {
			CHECK(&table->device == &device, "Vulkan descriptor table belongs to another device");
			current_command->trackResource(table);
			descriptor_sets.push_back(table->getHandle());
			continue;
		}

		auto* vk_set = dynamic_cast<VulkanBindingSet*>(set.get());
		CHECK(vk_set, "A Vulkan binding requires a Vulkan binding set");
		current_command->trackResource(vk_set);
		descriptor_sets.push_back(vk_set->getHandle());
	}

	if (!descriptor_sets.empty()) {
		vk::BindDescriptorSetsInfo binding_info{};
		binding_info.setStageFlags(stages)
		    .setLayout(current_layout)
		    .setFirstSet(0)
		    .setDescriptorSets(descriptor_sets);
		current_command->getHandle().bindDescriptorSets2(binding_info);
	}
}

void VulkanCommandList::beginRenderPass(RHIFramebuffer& framebuffer)
{
	auto* vk_framebuffer = dynamic_cast<VulkanFramebuffer*>(&framebuffer);
	CHECK(vk_framebuffer, "Vulkan rendering requires a Vulkan framebuffer");

	for (const auto& attachment : vk_framebuffer->getDesc().color_attachments)
		if (attachment.texture_view)
			validateRHITextureState(attachment.texture_view->getTexture(), RenderTarget, "beginRendering");

	const auto& framebuffer_depth = vk_framebuffer->getDesc().depth_attachment;
	if (framebuffer_depth.texture_view)
		validateRHITextureState(
		    framebuffer_depth.texture_view->getTexture(),
		    framebuffer_depth.read_only ? DepthRead : DepthWrite,
		    "beginRendering");

	graphics_state.framebuffer = &framebuffer;
	current_command->trackResource(&framebuffer);

	const auto& color_attachments = vk_framebuffer->getColorAttachmentsInfo();
	const auto& depth_attachment = vk_framebuffer->getDepthAttachmentInfo();
	const auto& stencil_attachment = vk_framebuffer->getStencilAttachmentInfo();

	vk::RenderingInfo render_info{};
	render_info.setRenderArea(
	               vk::Rect2D({0, 0},
	                   vk::Extent2D{framebuffer.getDesc().width, framebuffer.getDesc().height}))
	    .setLayerCount(framebuffer.getDesc().array_size)
	    .setColorAttachments(color_attachments)
	    .setPDepthAttachment(depth_attachment.imageView ? &depth_attachment : nullptr)
	    .setPStencilAttachment(stencil_attachment.imageView ? &stencil_attachment : nullptr);

	current_command->getHandle().beginRendering(render_info);
	rendering = true;
}

void VulkanCommandList::endRenderPass()
{
	if (current_command && rendering) {
		current_command->getHandle().endRendering();
		graphics_state.framebuffer = nullptr;
		rendering = false;
	}
}

void VulkanCommandList::open()
{
	CHECK(!recording, "Vulkan command list is already open");
	CHECK(!current_command || current_command->submitted, "Cannot discard an unsubmitted Vulkan command list");
	current_command.reset();
	current_command = device.getQueue().acquire();

	vk::CommandBufferBeginInfo begin_info{};
	begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	current_command->getHandle().begin(begin_info);
	recording = true;

	clear();
}

void VulkanCommandList::close()
{
	CHECK(recording && current_command, "Vulkan command list is not open");
	endRenderPass();
	current_command->getHandle().end();
	current_command->closed = true;
	recording = false;
	clear();
}

void VulkanCommandList::clear()
{
	endRenderPass();

	graphics_state = {};
	compute_state = {};
	current_layout = vk::PipelineLayout{};
	current_push_constant_visibility = RHIShaderType::None;
	current_push_constant_size = 0;
	rendering = false;
}

void VulkanCommandList::setPushConstants(const void* data, size_t size)
{
	CHECK(recording && current_command, "Cannot push constants without an active command buffer");
	CHECK(current_push_constant_visibility != RHIShaderType::None, "The active pipeline has no push constant binding");
	CHECK(data && size != 0 && size % 4 == 0 && size <= current_push_constant_size, "Push constant data must be non-empty and 4-byte aligned");

	vk::PushConstantsInfo info{};
	info.setLayout(current_layout)
	    .setStageFlags(toVkShaderStageFlags(current_push_constant_visibility))
	    .setSize(static_cast<uint32>(size))
	    .setPValues(data);
	current_command->getHandle().pushConstants2(info);
}

void VulkanCommandList::draw(const RHIDrawArguments& args)
{
	CHECK(recording && current_command, "Vulkan command list is not open");
	CHECK(graphics_state.pipeline && rendering, "Draw requires graphics state and a framebuffer");

	current_command->getHandle().draw(
	    args.vertex_count,
	    args.instance_count,
	    args.start_vertex,
	    args.start_instance);
}

void VulkanCommandList::drawIndexed(const RHIDrawArguments& args)
{
	CHECK(recording && current_command, "Vulkan command list is not open");
	CHECK(graphics_state.pipeline && rendering && graphics_state.index_buffer.buffer, "Indexed draw requires graphics state, framebuffer and index buffer");

	current_command->getHandle().drawIndexed(
	    args.vertex_count,
	    args.instance_count,
	    args.start_index,
	    args.start_vertex,
	    args.start_instance);
}

void VulkanCommandList::dispatch(uint32 group_count_x, uint32 group_count_y, uint32 group_count_z)
{
	CHECK(recording && current_command, "Cannot dispatch without an active command buffer");
	CHECK(compute_state.pipeline, "Cannot dispatch without a bound compute pipeline");
	CHECK(group_count_x != 0 && group_count_y != 0 && group_count_z != 0 &&
	        group_count_x <= 65535 && group_count_y <= 65535 && group_count_z <= 65535,
	    "Compute dispatch group counts must be greater than zero");
	endRenderPass();
	current_command->getHandle().dispatch(group_count_x, group_count_y, group_count_z);
}

void VulkanCommandList::beginRendering(RHIFramebuffer* framebuffer)
{
	CHECK(recording && current_command, "Cannot begin rendering without an active command buffer");
	CHECK(!rendering, "A Vulkan render pass is already active");
	CHECK(framebuffer, "A framebuffer to begin rendering cannot be null");
	endRenderPass();
	beginRenderPass(*framebuffer);
}

void VulkanCommandList::endRendering()
{
	endRenderPass();
}

void VulkanCommandList::setGraphicsState(const RHIGraphicsState& state)
{
	CHECK(recording && current_command, "Cannot set graphics state without an active command buffer");

	CHECK(state.framebuffer, "Graphics state requires a framebuffer");
	CHECK(!rendering || graphics_state.framebuffer == state.framebuffer, "Cannot change framebuffer inside a render pass");
	if (!rendering)
		beginRenderPass(*state.framebuffer);

	auto* vk_pipeline = dynamic_cast<VulkanGraphicsPipeline*>(state.pipeline.get());
	CHECK(vk_pipeline, "Vulkan graphics state requires a Vulkan graphics pipeline");
	const auto viewport_state = normalizeRHIViewportState(state.viewport_state, state.framebuffer->getDesc());

	current_command->trackResource(state.pipeline.get());
	current_command->trackResource(state.framebuffer.get());

	graphics_state = state;
	compute_state = {};
	current_layout = vk_pipeline->getLayout();
	current_push_constant_visibility = vk_pipeline->getPushConstantVisibility();
	current_push_constant_size = vk_pipeline->getPushConstantSize();

	current_command->getHandle().bindPipeline(
	    vk::PipelineBindPoint::eGraphics,
	    vk_pipeline->getHandle());

	const auto command = current_command->getHandle();
	const auto& pipeline_desc = vk_pipeline->getDesc();
	const auto& raster = pipeline_desc.raster_state;
	const auto& depth = pipeline_desc.depth_state;
	const auto& front = depth.front_face_stencil;
	const auto& back = depth.back_face_stencil;
	command.setCullMode(toVkCullMode(raster.cull_mode));
	command.setFrontFace(toVkFrontFace(raster.front_face));
	command.setPrimitiveTopology(toVkPrimitiveTopology(pipeline_desc.primitive_type));
	command.setPrimitiveRestartEnable(false);
	command.setRasterizerDiscardEnable(raster.rasterizer_discard_enable);
	command.setDepthBiasEnable(raster.depth_bias != 0 || raster.depth_bias_clamp != 0.0f || raster.depth_bias_slope_factor != 0.0f);
	command.setDepthBias(static_cast<float>(raster.depth_bias), raster.depth_bias_clamp, raster.depth_bias_slope_factor);
	command.setDepthTestEnable(depth.depth_test_enable);
	command.setDepthWriteEnable(depth.depth_write_enable);
	command.setDepthCompareOp(toVkCompareOp(depth.depth_compare_op));
	command.setDepthBoundsTestEnable(depth.depth_bounds_test_enable);
	command.setStencilTestEnable(depth.stencil_test_enable);
	command.setStencilOp(vk::StencilFaceFlagBits::eFront, toVkStencilOp(front.fail_op),
	    toVkStencilOp(front.pass_op), toVkStencilOp(front.depth_fail_op), toVkCompareOp(front.stencil_func));
	command.setStencilOp(vk::StencilFaceFlagBits::eBack, toVkStencilOp(back.fail_op),
	    toVkStencilOp(back.pass_op), toVkStencilOp(back.depth_fail_op), toVkCompareOp(back.stencil_func));
	std::vector<vk::Viewport> viewports;
	viewports.reserve(viewport_state.viewports.size());
	for (const auto& viewport : viewport_state.viewports)
		viewports.push_back(toVkViewport(viewport));
	if (!viewports.empty())
		current_command->getHandle().setViewport(0, viewports);

	std::vector<vk::Rect2D> scissors;
	scissors.reserve(viewport_state.scissors.size());
	for (const auto& scissor : viewport_state.scissors)
		scissors.push_back(toVkRect2D(scissor));
	if (!scissors.empty())
		current_command->getHandle().setScissor(0, scissors);

	bindSets(state.binding_sets, pipeline_desc.binding_layouts, vk::ShaderStageFlagBits::eAllGraphics);

	for (const auto& binding : state.vertex_buffers) {
		auto* buffer = dynamic_cast<VulkanBuffer*>(binding.buffer.get());
		CHECK(buffer && state.pipeline->getDesc().input_layout && binding.offset < buffer->getDesc().size, "Invalid vertex buffer binding");
		const auto* input_layout = dynamic_cast<VulkanInputLayout*>(state.pipeline->getDesc().input_layout.get());
		CHECK(input_layout && std::ranges::any_of(input_layout->getBindings(), [&](const auto& input) { return input.binding == binding.slot; }),
		    "Vertex buffer slot is absent from the input layout");
		useBuffer(buffer, VertexBuffer, "bind vertex buffer");
		current_command->getHandle().bindVertexBuffers(binding.slot, buffer->getHandle(), {binding.offset});
	}
	if (state.index_buffer.buffer) {
		auto* buffer = dynamic_cast<VulkanBuffer*>(state.index_buffer.buffer.get());
		CHECK(buffer && state.index_buffer.offset < buffer->getDesc().size &&
		        (state.index_buffer.format == RHIFormat::R16_UINT || state.index_buffer.format == RHIFormat::R32_UINT),
		    "Invalid index buffer binding");
		useBuffer(buffer, IndexBuffer, "bind index buffer");
		current_command->getHandle().bindIndexBuffer(buffer->getHandle(), state.index_buffer.offset, toVkIndexType(state.index_buffer.format));
	}
}

void VulkanCommandList::setComputeState(const RHIComputeState& state)
{
	CHECK(recording && current_command, "Cannot set compute state without an active command buffer");
	endRenderPass();

	auto* vk_pipeline = dynamic_cast<VulkanComputePipeline*>(state.pipeline.get());
	CHECK(vk_pipeline, "Vulkan compute state requires a Vulkan compute pipeline");

	current_command->trackResource(state.pipeline.get());

	compute_state = state;
	graphics_state = {};
	current_layout = vk_pipeline->getLayout();
	current_push_constant_visibility = vk_pipeline->getPushConstantVisibility();
	current_push_constant_size = vk_pipeline->getPushConstantSize();

	current_command->getHandle().bindPipeline(
	    vk::PipelineBindPoint::eCompute,
	    vk_pipeline->getHandle());

	bindSets(state.binding_sets, vk_pipeline->getDesc().binding_layouts, vk::ShaderStageFlagBits::eCompute);
}

void VulkanCommandList::beginDebugLabel(std::string_view name)
{
	CHECK(recording && current_command, "Vulkan command list is not open");
	device.beginDebugLabel(current_command->getHandle(), name);
}

void VulkanCommandList::endDebugLabel()
{
	CHECK(recording && current_command, "Vulkan command list is not open");
	device.endDebugLabel(current_command->getHandle());
}

}        // namespace Vortex

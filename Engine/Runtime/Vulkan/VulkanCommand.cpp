module Runtime.Vulkan;

import vulkan;

namespace Vortex {

static void validateTextureState(const RHITexture& texture, RHIResourceState expected, const char* operation)
{
#if VDEBUG
	CHECK(texture.getState() == expected,
	    "Vulkan texture '{}' is in state {} but {} requires state {}",
	    texture.getName(),
	    static_cast<uint16>(texture.getState()),
	    operation,
	    static_cast<uint16>(expected));
#endif
}

static void validateBufferState(const RHIBuffer& buffer, RHIResourceState expected, const char* operation)
{
#if VDEBUG
	CHECK(buffer.getState() == expected,
	    "Vulkan buffer '{}' is in state {} but {} requires state {}",
	    buffer.getName(),
	    static_cast<uint16>(buffer.getState()),
	    operation,
	    static_cast<uint16>(expected));
#endif
}

static uint32 getMipExtent(uint32 extent, uint32 mip_level)
{
	return std::max(1u, extent >> mip_level);
}

static RHITextureSlice normalizeTextureSlice(const RHITextureDesc& desc, const RHITextureSlice& slice)
{
	CHECK(slice.mip_level < desc.mip_levels && slice.array_layer < desc.array_layers,
	    "Texture slice selects a subresource outside the texture");
	CHECK(slice.x >= 0 && slice.y >= 0 && slice.z >= 0,
	    "Texture slice offsets cannot be negative");

	const auto mip_width = getMipExtent(desc.width, slice.mip_level);
	const auto mip_height = getMipExtent(desc.height, slice.mip_level);
	const auto mip_depth = getMipExtent(desc.depth, slice.mip_level);
	const bool origin_in_bounds =
	    static_cast<uint32>(slice.x) < mip_width &&
	    static_cast<uint32>(slice.y) < mip_height &&
	    static_cast<uint32>(slice.z) < mip_depth;
	CHECK(origin_in_bounds,
	    "Texture slice offset is outside the selected mip");

	auto result = slice;
	result.width = slice.width < 0 ? static_cast<int>(mip_width) - slice.x : slice.width;
	result.height = slice.height < 0 ? static_cast<int>(mip_height) - slice.y : slice.height;
	result.depth = slice.depth < 0 ? static_cast<int>(mip_depth) - slice.z : slice.depth;
	const bool extent_in_bounds =
	    result.width > 0 && result.height > 0 && result.depth > 0 &&
	    static_cast<uint32>(slice.x + result.width) <= mip_width &&
	    static_cast<uint32>(slice.y + result.height) <= mip_height &&
	    static_cast<uint32>(slice.z + result.depth) <= mip_depth;
	CHECK(extent_in_bounds, "Texture slice extent is outside the selected mip");

	return result;
}

RHIRef<RHICommandList> VulkanDevice::createCommandList(const RHICommandListDesc& desc)
{
	CHECK(desc.queue_type == RHICommandQueue::Graphics,
	    "Only the graphics Vulkan queue is currently initialized");

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
	allocMemory();
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
	freeMemory();
}

void VulkanCommandBuffer::allocMemory()
{
	vk::CommandBufferAllocateInfo buffer_info{};
	buffer_info.setCommandPool(pool.getHandle())
	    .setLevel(vk::CommandBufferLevel::ePrimary)
	    .setCommandBufferCount(1);

	buffer = device.getHandle().allocateCommandBuffers(buffer_info).front();
}

void VulkanCommandBuffer::freeMemory()
{
	device.getHandle().freeCommandBuffers(pool.getHandle(), buffer);
	buffer = nullptr;
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

void VulkanCommandBuffer::resetResources()
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
	for (auto* cmd_buffer : cmd_buffers)
		delete cmd_buffer;

	for (auto* cmd_buffer : free_cmd_buffers)
		delete cmd_buffer;

	device.getHandle().destroyCommandPool(pool);
	pool = nullptr;
}

VulkanCommandBuffer* VulkanCommandPool::createCommandBuffer()
{
	if (!free_cmd_buffers.empty()) {
		auto* cmd_buffer = free_cmd_buffers.front();
		free_cmd_buffers.pop_front();

		cmd_buffer->allocMemory();
		cmd_buffers.push_back(cmd_buffer);

		return cmd_buffer;
	}

	auto* cmd_buffer = new VulkanCommandBuffer(device, *this);
	cmd_buffers.push_back(cmd_buffer);

	return cmd_buffer;
}

void VulkanCommandPool::releaseCommandBuffer(VulkanCommandBuffer* cmd_buffer)
{
	auto it = std::find(cmd_buffers.begin(), cmd_buffers.end(), cmd_buffer);
	if (it == cmd_buffers.end())
		return;

	cmd_buffers.erase(it);
	cmd_buffer->freeMemory();
	cmd_buffer->resetResources();
	free_cmd_buffers.push_back(cmd_buffer);
}


void VulkanCommandList::beginRenderPass(RHIFramebuffer& framebuffer)
{
	auto* vk_framebuffer = dynamic_cast<VulkanFramebuffer*>(&framebuffer);
	CHECK(vk_framebuffer, "Vulkan rendering requires a Vulkan framebuffer");
	CHECK(current_command, "Cannot begin rendering without an active command buffer");

	for (const auto& attachment : vk_framebuffer->getDesc().color_attachments)
		if (attachment.texture_view)
			validateTextureState(attachment.texture_view->getTexture(), RenderTarget, "beginRendering");

	const auto& framebuffer_depth = vk_framebuffer->getDesc().depth_attachment;
	if (framebuffer_depth.texture_view)
		validateTextureState(
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
	current_command = device.getQueue().acquireCommand();

	vk::CommandBufferBeginInfo begin_info{};
	begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	current_command->getHandle().begin(begin_info);

	clear();
}

void VulkanCommandList::close()
{
	endRenderPass();
	current_command->getHandle().end();

	clear();
}

void VulkanCommandList::clear()
{
	endRenderPass();

	graphics_state = {};
	compute_state = {};
	current_layout = vk::PipelineLayout{};
	current_push_constant_visibility = RHIShaderType::None;
	rendering = false;
}

void VulkanCommandList::clearTexture(RHITexture* texture, const RHIColor& color)
{
	endRenderPass();

	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	CHECK(vk_texture, "A texture to clear cannot be null");
	CHECK(current_command, "Cannot clear a texture without an active command buffer");

	validateTextureState(*texture, CopyDest, "clearTexture");

	vk::ClearColorValue clear_value = toVkClearColorValue(color);
	vk::ImageSubresourceRange subresource{};
	subresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
	    .setBaseMipLevel(0)
	    .setLevelCount(texture->getDesc().mip_levels)
	    .setBaseArrayLayer(0)
	    .setLayerCount(texture->getDesc().array_layers);

	current_command->getHandle().clearColorImage(vk_texture->getHandle(),
	    vk::ImageLayout::eTransferDstOptimal,
	    &clear_value,
	    1,
	    &subresource);
}

void VulkanCommandList::clearDepthTexture(RHITexture* texture, bool clear_depth, float depth, bool clear_stencil, uint8 stencil)
{
	endRenderPass();

	if (!clear_depth && !clear_stencil)
		return;

	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	CHECK(vk_texture, "A texture to clear cannot be null");
	CHECK(current_command, "Cannot clear a texture without an active command buffer");

	validateTextureState(*texture, CopyDest, "clearDepthTexture");

	vk::ImageAspectFlags aspect{};
	if (clear_depth)
		aspect |= vk::ImageAspectFlagBits::eDepth;
	if (clear_stencil)
		aspect |= vk::ImageAspectFlagBits::eStencil;

	vk::ClearDepthStencilValue clear_value = {depth, stencil};
	vk::ImageSubresourceRange subresource{};
	subresource.setAspectMask(aspect)
	    .setBaseMipLevel(0)
	    .setLevelCount(vk_texture->getDesc().mip_levels)
	    .setBaseArrayLayer(0)
	    .setLayerCount(vk_texture->getDesc().array_layers);

	current_command->getHandle().clearDepthStencilImage(vk_texture->getHandle(),
	    vk::ImageLayout::eTransferDstOptimal,
	    &clear_value,
	    1,
	    &subresource);
}

void VulkanCommandList::copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice)
{
	endRenderPass();

	auto* vk_dst_texture = static_cast<VulkanTexture*>(dst_texture);
	auto* vk_src_texture = static_cast<VulkanTexture*>(src_texture);
	CHECK(vk_dst_texture && vk_src_texture,
	    "Source and destination textures for copy cannot be null");
	CHECK(current_command,
	    "Cannot copy textures without an active command buffer");

	validateTextureState(*src_texture, CopySource, "copyTexture source");
	validateTextureState(*dst_texture, CopyDest, "copyTexture destination");
	const auto normalized_src = normalizeTextureSlice(src_texture->getDesc(), src_slice);
	const auto normalized_dst = normalizeTextureSlice(dst_texture->getDesc(), dst_slice);

	vk::ImageSubresourceLayers src_subresource{};
	src_subresource.setAspectMask(getVkImageAspectFlags(src_texture->getDesc().format))
	    .setMipLevel(normalized_src.mip_level)
	    .setBaseArrayLayer(normalized_src.array_layer)
	    .setLayerCount(1);

	vk::ImageSubresourceLayers dst_subresource{};
	dst_subresource.setAspectMask(getVkImageAspectFlags(dst_texture->getDesc().format))
	    .setMipLevel(normalized_dst.mip_level)
	    .setBaseArrayLayer(normalized_dst.array_layer)
	    .setLayerCount(1);

	vk::Extent3D mip_extent = {
	    static_cast<uint32>(std::min(normalized_src.width, normalized_dst.width)),
	    static_cast<uint32>(std::min(normalized_src.height, normalized_dst.height)),
	    static_cast<uint32>(std::min(normalized_src.depth, normalized_dst.depth))};

	vk::ImageCopy region{};
	region.setSrcSubresource(src_subresource)
	    .setSrcOffset(vk::Offset3D(normalized_src.x, normalized_src.y, normalized_src.z))
	    .setDstSubresource(dst_subresource)
	    .setDstOffset(vk::Offset3D(normalized_dst.x, normalized_dst.y, normalized_dst.z))
	    .setExtent(mip_extent);

	current_command->getHandle().copyImage(
	    vk_src_texture->getHandle(),
	    vk::ImageLayout::eTransferSrcOptimal,
	    vk_dst_texture->getHandle(),
	    vk::ImageLayout::eTransferDstOptimal,
	    region);
}

void VulkanCommandList::copyTexture(RHIStagingTexture* dst_staging, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice)
{
	endRenderPass();

	auto* vk_src_texture = static_cast<VulkanTexture*>(src_texture);
	auto* vk_dst_staging = static_cast<VulkanStagingTexture*>(dst_staging);
	CHECK(vk_src_texture && vk_dst_staging, "Source texture and destination staging buffer for copy cannot be null");
	CHECK(current_command, "Cannot copy a texture to a staging buffer without an active command buffer");
	current_command->trackResource(dst_staging);

	validateTextureState(*src_texture, CopySource, "copyTexture source");
	const auto normalized_src = normalizeTextureSlice(src_texture->getDesc(), src_slice);

	vk::ImageSubresourceLayers src_subresource{};
	src_subresource.setAspectMask(getVkImageAspectFlags(src_texture->getDesc().format))
	    .setMipLevel(normalized_src.mip_level)
	    .setBaseArrayLayer(normalized_src.array_layer)
	    .setLayerCount(1);

	vk::BufferImageCopy region{};
	region.setBufferOffset(0)
	    .setBufferRowLength(0)
	    .setBufferImageHeight(0)
	    .setImageSubresource(src_subresource)
	    .setImageOffset(vk::Offset3D(normalized_src.x, normalized_src.y, normalized_src.z))
	    .setImageExtent(vk::Extent3D(
	        static_cast<uint32>(normalized_src.width),
	        static_cast<uint32>(normalized_src.height),
	        static_cast<uint32>(normalized_src.depth)));

	current_command->getHandle().copyImageToBuffer(
	    vk_src_texture->getHandle(),
	    vk::ImageLayout::eTransferSrcOptimal,
	    vk_dst_staging->getHandle(),
	    region);
}

void VulkanCommandList::copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHIStagingTexture* src_staging, const RHITextureSlice& src_slice)
{
	endRenderPass();

	auto* vk_dst_texture = static_cast<VulkanTexture*>(dst_texture);
	auto* vk_src_staging = static_cast<VulkanStagingTexture*>(src_staging);
	CHECK(vk_dst_texture && vk_src_staging, "Source staging buffer and destination texture for copy cannot be null");
	CHECK(current_command, "Cannot copy a staging buffer to a texture without an active command buffer");
	current_command->trackResource(src_staging);

	validateTextureState(*dst_texture, CopyDest, "copyTexture destination");
	const auto normalized_dst = normalizeTextureSlice(dst_texture->getDesc(), dst_slice);

	vk::ImageSubresourceLayers dst_subresource{};
	dst_subresource.setAspectMask(getVkImageAspectFlags(dst_texture->getDesc().format))
	    .setMipLevel(normalized_dst.mip_level)
	    .setBaseArrayLayer(normalized_dst.array_layer)
	    .setLayerCount(1);

	vk::BufferImageCopy region{};
	region.setBufferOffset(0)
	    .setBufferRowLength(0)
	    .setBufferImageHeight(0)
	    .setImageSubresource(dst_subresource)
	    .setImageOffset(vk::Offset3D(normalized_dst.x, normalized_dst.y, normalized_dst.z))
	    .setImageExtent(vk::Extent3D(
	        static_cast<uint32>(normalized_dst.width),
	        static_cast<uint32>(normalized_dst.height),
	        static_cast<uint32>(normalized_dst.depth)));

	current_command->getHandle().copyBufferToImage(
	    vk_src_staging->getHandle(),
	    vk_dst_texture->getHandle(),
	    vk::ImageLayout::eTransferDstOptimal,
	    region);
}

void VulkanCommandList::writeTexture(RHITexture* texture, const RHITextureSlice& slice, const void* data, uint64 size)
{
	endRenderPass();

	if (size == 0)
		return;

	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	CHECK(vk_texture, "A texture to write cannot be null");

	auto staging_texture = device.createStagingTexture(texture->getDesc());

	void* mapped = device.mapStagingTexture(staging_texture.get(), RHIAccessMode::Write);
	std::memcpy(mapped, data, static_cast<size_t>(size));
	device.unmapStagingTexture(staging_texture.get());

	transitionTexture(texture, texture->getState(), CopyDest);
	copyTexture(texture, slice, staging_texture.get(), slice);

	current_command->trackResource(staging_texture.get());
}

void VulkanCommandList::clearBuffer(RHIBuffer* buffer, uint32 value)
{
	endRenderPass();

	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	CHECK(vk_buffer, "A buffer to clear cannot be null");
	CHECK(current_command, "Cannot clear a buffer without an active command buffer");

	validateBufferState(*buffer, CopyDest, "clearBuffer");

	current_command->getHandle().fillBuffer(
	    vk_buffer->getHandle(),
	    0,
	    vk_buffer->getDesc().size,
	    value);
}

void VulkanCommandList::copyBuffer(RHIBuffer* dst_buffer, uint64 dst_offset, RHIBuffer* src_buffer, uint64 src_offset, uint64 size)
{
	endRenderPass();

	auto* vk_dst_buffer = static_cast<VulkanBuffer*>(dst_buffer);
	auto* vk_src_buffer = static_cast<VulkanBuffer*>(src_buffer);
	CHECK(vk_dst_buffer && vk_src_buffer, "Source and destination buffers for copy cannot be null");
	CHECK(current_command, "Cannot copy buffers without an active command buffer");

	validateBufferState(*src_buffer, CopySource, "copyBuffer source");
	validateBufferState(*dst_buffer, CopyDest, "copyBuffer destination");

	vk::BufferCopy region{};
	region.setSrcOffset(src_offset)
	    .setDstOffset(dst_offset)
	    .setSize(size);

	current_command->getHandle().copyBuffer(
	    vk_src_buffer->getHandle(),
	    vk_dst_buffer->getHandle(),
	    region);
}

void VulkanCommandList::writeBuffer(RHIBuffer* buffer, uint64 offset, const void* data, uint64 size)
{
	endRenderPass();

	if (size == 0)
		return;

	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	CHECK(vk_buffer, "A buffer to write cannot be null");

	RHIBufferDesc staging_desc{};
	staging_desc.setSize(size)
	    .setUsage(RHIBufferUsage::CopySource)
	    .setAccess(RHIAccessMode::Write);

	auto staging_buffer = device.createBuffer(staging_desc);

	void* mapped = device.mapBuffer(staging_buffer.get(), RHIAccessMode::Write);
	std::memcpy(mapped, data, static_cast<size_t>(size));
	device.unmapBuffer(staging_buffer.get());

	transitionBuffer(staging_buffer.get(), Unknown, CopySource);
	transitionBuffer(buffer, buffer->getState(), CopyDest);
	copyBuffer(buffer, offset, staging_buffer.get(), 0, size);

	current_command->trackResource(staging_buffer.get());
}

void VulkanCommandList::setPushConstants(const void* data, size_t size)
{
	CHECK(current_command, "Cannot push constants without an active command buffer");
	CHECK(current_layout, "Cannot push constants without a bound pipeline");
	CHECK(current_push_constant_visibility != RHIShaderType::None, "The active pipeline has no push constant binding");
	CHECK(data && size != 0 && size % 4 == 0, "Push constant data must be non-empty and 4-byte aligned");

	current_command->getHandle().pushConstants(
	    current_layout,
	    toVkShaderStageFlags(current_push_constant_visibility),
	    0,
	    static_cast<uint32>(size),
	    data);
}

void VulkanCommandList::draw(const RHIDrawArguments& args)
{
	CHECK(current_command, "Cannot draw without an active command buffer");

	current_command->getHandle().draw(
	    args.vertex_count,
	    args.instance_count,
	    args.start_vertex,
	    args.start_instance);
}

void VulkanCommandList::drawIndexed(const RHIDrawArguments& args)
{
	CHECK(current_command, "Cannot draw indexed without an active command buffer");

	current_command->getHandle().drawIndexed(
	    args.vertex_count,
	    args.instance_count,
	    args.start_index,
	    args.start_vertex,
	    args.start_instance);
}

void VulkanCommandList::transitionTexture(RHITexture* texture, RHIResourceState before, RHIResourceState after)
{
	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	CHECK(vk_texture, "A texture to transition cannot be null");
	CHECK(current_command, "Cannot transition a texture without an active command buffer");
	current_command->trackResource(texture);
	validateTextureState(*texture, before, "transitionTexture before state");

	auto old_info = getTextureTransition(before);
	auto new_info = getTextureTransition(after);

	vk::ImageMemoryBarrier2 barrier{};
	barrier.setOldLayout(vk_texture->layout)
	    .setNewLayout(new_info.layout)
	    .setSrcStageMask(old_info.stage)
	    .setDstStageMask(new_info.stage)
	    .setSrcAccessMask(old_info.access)
	    .setDstAccessMask(new_info.access)
	    .setImage(vk_texture->getHandle())
	    .setSubresourceRange(vk::ImageSubresourceRange()
	            .setAspectMask(getVkImageAspectFlags(vk_texture->getDesc().format))
	            .setBaseMipLevel(0)
	            .setLevelCount(vk_texture->getDesc().mip_levels)
	            .setBaseArrayLayer(0)
	            .setLayerCount(vk_texture->getDesc().array_layers));

	vk::DependencyInfo dependency{};
	dependency.setImageMemoryBarriers(barrier);

	current_command->getHandle().pipelineBarrier2(dependency);
	vk_texture->layout = new_info.layout;
	vk_texture->state = after;
}

void VulkanCommandList::dispatch(uint32 group_count_x, uint32 group_count_y, uint32 group_count_z)
{
	CHECK(current_command, "Cannot dispatch without an active command buffer");
	CHECK(compute_state.pipeline, "Cannot dispatch without a bound compute pipeline");
	CHECK(group_count_x != 0 && group_count_y != 0 && group_count_z != 0,
	    "Compute dispatch group counts must be greater than zero");
	endRenderPass();
	current_command->getHandle().dispatch(group_count_x, group_count_y, group_count_z);
}

void VulkanCommandList::transitionBuffer(RHIBuffer* buffer, RHIResourceState before, RHIResourceState after)
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	CHECK(vk_buffer, "A buffer to transition cannot be null");
	CHECK(current_command, "Cannot transition a buffer without an active command buffer");
	current_command->trackResource(buffer);
	validateBufferState(*buffer, before, "transitionBuffer before state");

	const auto old_info = getBufferTransition(before);
	const auto new_info = getBufferTransition(after);

	vk::BufferMemoryBarrier2 barrier{};
	barrier.setSrcStageMask(old_info.stage)
	    .setDstStageMask(new_info.stage)
	    .setSrcAccessMask(old_info.access)
	    .setDstAccessMask(new_info.access)
	    .setBuffer(vk_buffer->getHandle())
	    .setOffset(0)
	    .setSize(vk_buffer->getDesc().size);

	vk::DependencyInfo dependency{};
	dependency.setBufferMemoryBarriers(barrier);

	current_command->getHandle().pipelineBarrier2(dependency);
	vk_buffer->state = after;
}

void VulkanCommandList::beginRendering(RHIFramebuffer* framebuffer)
{
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
	CHECK(current_command, "Cannot set graphics state without an active command buffer");

	if (state.framebuffer && (!rendering || graphics_state.framebuffer != state.framebuffer)) {
		endRenderPass();

		auto* vk_framebuffer = dynamic_cast<VulkanFramebuffer*>(state.framebuffer.get());
		CHECK(vk_framebuffer, "Vulkan graphics state requires a Vulkan framebuffer");

		beginRenderPass(*state.framebuffer);
	}

	// bind pipeline
	auto* vk_pipeline = dynamic_cast<VulkanGraphicsPipeline*>(state.pipeline.get());
	CHECK(vk_pipeline, "Vulkan graphics state requires a Vulkan graphics pipeline");

	current_command->trackResource(state.pipeline.get());
	current_command->trackResource(state.framebuffer.get());

	for (const auto& binding_set : state.binding_sets)
		current_command->trackResource(binding_set.get());

	for (const auto& vertex_buffer : state.vertex_buffers)
		current_command->trackResource(vertex_buffer.buffer.get());

	current_command->trackResource(state.index_buffer.buffer.get());

	graphics_state = state;
	compute_state = {};
	current_layout = vk_pipeline->getLayout();
	current_push_constant_visibility = vk_pipeline->getPushConstantVisibility();

	current_command->getHandle().bindPipeline(
	    vk::PipelineBindPoint::eGraphics,
	    vk_pipeline->getHandle());

	// Set dynamic states
	std::vector<vk::Viewport> viewports;
	viewports.reserve(state.viewport_state.viewports.size());
	for (const auto& viewport : state.viewport_state.viewports)
		viewports.push_back(toVkViewport(viewport));
	if (!viewports.empty())
		current_command->getHandle().setViewport(0, viewports);

	std::vector<vk::Rect2D> scissors;
	scissors.reserve(state.viewport_state.scissors.size());
	for (const auto& scissor : state.viewport_state.scissors)
		scissors.push_back(toVkRect2D(scissor));
	if (!scissors.empty())
		current_command->getHandle().setScissor(0, scissors);

	// bind descriptor sets
	std::vector<vk::DescriptorSet> descriptor_sets;
	descriptor_sets.reserve(state.binding_sets.size());
	for (const auto& set : state.binding_sets)
		if (auto* vk_set = dynamic_cast<VulkanBindingSet*>(set.get()))
			descriptor_sets.push_back(vk_set->getHandle());
	if (!descriptor_sets.empty())
		current_command->getHandle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, current_layout, 0, descriptor_sets, {});

	// bind vertex and index buffers
	for (const auto& binding : state.vertex_buffers) {
		if (auto* vertex_buffer = dynamic_cast<VulkanBuffer*>(binding.buffer.get())) {
			current_command->getHandle().bindVertexBuffers(binding.slot, vertex_buffer->getHandle(), {binding.offset});
		}
	}

	if (auto* index_buffer = dynamic_cast<VulkanBuffer*>(state.index_buffer.buffer.get())) {
		CHECK(state.index_buffer.format == RHIFormat::R16_UINT || state.index_buffer.format == RHIFormat::R32_UINT,
		    "An index buffer requires R16_UINT or R32_UINT format");
		current_command->getHandle().bindIndexBuffer(index_buffer->getHandle(), state.index_buffer.offset, toVkIndexType(state.index_buffer.format));
	}
}

void VulkanCommandList::setComputeState(const RHIComputeState& state)
{
	CHECK(current_command, "Cannot set compute state without an active command buffer");
	endRenderPass();

	auto* vk_pipeline = dynamic_cast<VulkanComputePipeline*>(state.pipeline.get());
	CHECK(vk_pipeline, "Vulkan compute state requires a Vulkan compute pipeline");

	current_command->trackResource(state.pipeline.get());
	for (const auto& binding_set : state.binding_sets)
		current_command->trackResource(binding_set.get());

	compute_state = state;
	graphics_state = {};
	current_layout = vk_pipeline->getLayout();
	current_push_constant_visibility = vk_pipeline->getPushConstantVisibility();

	current_command->getHandle().bindPipeline(
	    vk::PipelineBindPoint::eCompute,
	    vk_pipeline->getHandle());

	std::vector<vk::DescriptorSet> descriptor_sets;
	descriptor_sets.reserve(state.binding_sets.size());
	for (const auto& set : state.binding_sets)
		if (auto* vk_set = dynamic_cast<VulkanBindingSet*>(set.get()))
			descriptor_sets.push_back(vk_set->getHandle());
	if (!descriptor_sets.empty())
		current_command->getHandle().bindDescriptorSets(
		    vk::PipelineBindPoint::eCompute, current_layout, 0, descriptor_sets, {});
}

void VulkanCommandList::beginDebugLabel(std::string_view name)
{
	if (current_command)
		device.beginDebugLabel(current_command->getHandle(), name);
}

void VulkanCommandList::endDebugLabel()
{
	if (current_command)
		device.endDebugLabel(current_command->getHandle());
}

}        // namespace Vortex

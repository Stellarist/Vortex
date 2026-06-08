#include "VulkanCommand.hpp"

#include "VulkanTypes.hpp"
#include "VulkanResources.hpp"
#include "VulkanQueue.hpp"
#include "VulkanDescriptor.hpp"
#include "VulkanPipeline.hpp"

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

void VulkanCommandBuffer::trackResource(std::shared_ptr<RHIResource> resource)
{
	if (resource)
		tracked_resources.push_back(std::move(resource));
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


void VulkanCommandList::beginRenderPass(RHIFrameBuffer& framebuffer)
{
	auto* vk_framebuffer = static_cast<VulkanFrameBuffer*>(&framebuffer);
	assert(vk_framebuffer && "A framebuffer to begin a render pass cannot be null.");

	graphics_state.framebuffer = &framebuffer;

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
	current_layout = vk::PipelineLayout{};
	current_visibility = vk::ShaderStageFlagBits{};
	rendering = false;
}

void VulkanCommandList::clearTexture(RHITexture* texture, const RHIColor& color)
{
	endRenderPass();

	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	assert(vk_texture && "A texture to clear cannot be null.");
	assert(current_command && "Cannot clear a texture without an active command buffer.");

	transitionTexture(texture, CopyDst);

	vk::ClearColorValue       clear_value = toVkClearColorValue(color);
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

void VulkanCommandList::clearDepthTexture(RHITexture* texture, bool clear_depth, float depth, bool clear_stencil, uint8_t stencil)
{
	endRenderPass();

	if (!clear_depth && !clear_stencil)
		return;

	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	assert(vk_texture && "A texture to clear cannot be null.");
	assert(current_command && "Cannot clear a texture without an active command buffer.");

	transitionTexture(texture, CopyDst);

	vk::ImageAspectFlags aspect{};
	if (clear_depth)
		aspect |= vk::ImageAspectFlagBits::eDepth;
	if (clear_stencil)
		aspect |= vk::ImageAspectFlagBits::eStencil;

	vk::ClearDepthStencilValue clear_value = {depth, stencil};
	vk::ImageSubresourceRange  subresource{};
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
	assert(vk_dst_texture && vk_src_texture && "Source and destination textures for copy cannot be null.");
	assert(current_command && "Cannot copy textures without an active command buffer.");

	transitionTexture(src_texture, CopySrc);
	transitionTexture(dst_texture, CopyDst);

	vk::ImageSubresourceLayers src_subresource{};
	src_subresource.setAspectMask(getVkImageAspectFlags(src_texture->getDesc().format))
	    .setMipLevel(src_slice.mip_level)
	    .setBaseArrayLayer(src_slice.array_layer)
	    .setLayerCount(1);

	vk::ImageSubresourceLayers dst_subresource{};
	dst_subresource.setAspectMask(getVkImageAspectFlags(dst_texture->getDesc().format))
	    .setMipLevel(dst_slice.mip_level)
	    .setBaseArrayLayer(dst_slice.array_layer)
	    .setLayerCount(1);

	vk::Extent3D mip_extent = {
	    static_cast<uint32_t>(std::min(src_slice.width, dst_slice.width)),
	    static_cast<uint32_t>(std::min(src_slice.height, dst_slice.height)),
	    static_cast<uint32_t>(std::min(src_slice.depth, dst_slice.depth))};

	vk::ImageCopy region{};
	region.setSrcSubresource(src_subresource)
	    .setSrcOffset(vk::Offset3D(src_slice.x, src_slice.y, src_slice.z))
	    .setDstSubresource(dst_subresource)
	    .setDstOffset(vk::Offset3D(dst_slice.x, dst_slice.y, dst_slice.z))
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
	assert(vk_src_texture && vk_dst_staging && "Source texture and destination staging buffer for copy cannot be null.");
	assert(current_command && "Cannot copy a texture to a staging buffer without an active command buffer.");

	transitionTexture(src_texture, CopySrc);

	vk::ImageSubresourceLayers src_subresource{};
	src_subresource.setAspectMask(getVkImageAspectFlags(src_texture->getDesc().format))
	    .setMipLevel(src_slice.mip_level)
	    .setBaseArrayLayer(src_slice.array_layer)
	    .setLayerCount(1);

	vk::BufferImageCopy region{};
	region.setBufferOffset(0)
	    .setBufferRowLength(0)
	    .setBufferImageHeight(0)
	    .setImageSubresource(src_subresource)
	    .setImageOffset(vk::Offset3D(src_slice.x, src_slice.y, src_slice.z))
	    .setImageExtent(vk::Extent3D(
	        src_slice.width < 0 ? src_texture->getDesc().width : static_cast<uint32_t>(src_slice.width),
	        src_slice.height < 0 ? src_texture->getDesc().height : static_cast<uint32_t>(src_slice.height),
	        src_slice.depth < 0 ? src_texture->getDesc().depth : static_cast<uint32_t>(src_slice.depth)));

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
	assert(vk_dst_texture && vk_src_staging && "Source staging buffer and destination texture for copy cannot be null.");
	assert(current_command && "Cannot copy a staging buffer to a texture without an active command buffer.");

	transitionTexture(dst_texture, CopyDst);

	vk::ImageSubresourceLayers dst_subresource{};
	dst_subresource.setAspectMask(getVkImageAspectFlags(dst_texture->getDesc().format))
	    .setMipLevel(dst_slice.mip_level)
	    .setBaseArrayLayer(dst_slice.array_layer)
	    .setLayerCount(1);

	vk::BufferImageCopy region{};
	region.setBufferOffset(0)
	    .setBufferRowLength(0)
	    .setBufferImageHeight(0)
	    .setImageSubresource(dst_subresource)
	    .setImageOffset(vk::Offset3D(dst_slice.x, dst_slice.y, dst_slice.z))
	    .setImageExtent(vk::Extent3D(
	        dst_slice.width < 0 ? dst_texture->getDesc().width : static_cast<uint32_t>(dst_slice.width),
	        dst_slice.height < 0 ? dst_texture->getDesc().height : static_cast<uint32_t>(dst_slice.height),
	        dst_slice.depth < 0 ? dst_texture->getDesc().depth : static_cast<uint32_t>(dst_slice.depth)));

	current_command->getHandle().copyBufferToImage(
	    vk_src_staging->getHandle(),
	    vk_dst_texture->getHandle(),
	    vk::ImageLayout::eTransferDstOptimal,
	    region);
}

void VulkanCommandList::writeTexture(RHITexture* texture, const RHITextureSlice& slice, const void* data, uint64_t size)
{
	endRenderPass();

	if (size == 0)
		return;

	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	assert(vk_texture && "A texture to write cannot be null.");

	auto staging_texture = device.createStagingTexture(texture->getDesc());

	void* mapped = device.mapStagingTexture(staging_texture.get(), RHIAccessMode::Write);
	std::memcpy(mapped, data, static_cast<size_t>(size));
	device.unmapStagingTexture(staging_texture.get());

	copyTexture(texture, slice, staging_texture.get(), slice);

	current_command->trackResource(std::move(staging_texture));
}

void VulkanCommandList::clearBuffer(RHIBuffer* buffer, uint32_t value)
{
	endRenderPass();

	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	assert(vk_buffer && "A buffer to clear cannot be null.");

	transitionBuffer(buffer, CopyDst);

	current_command->getHandle().fillBuffer(
	    vk_buffer->getHandle(),
	    0,
	    vk_buffer->getDesc().size,
	    value);
}

void VulkanCommandList::copyBuffer(RHIBuffer* dst_buffer, uint64_t dst_offset, RHIBuffer* src_buffer, uint64_t src_offset, uint64_t size)
{
	endRenderPass();

	auto* vk_dst_buffer = static_cast<VulkanBuffer*>(dst_buffer);
	auto* vk_src_buffer = static_cast<VulkanBuffer*>(src_buffer);
	assert(vk_dst_buffer && vk_src_buffer && "Source and destination buffers for copy cannot be null.");

	transitionBuffer(src_buffer, CopySrc);
	transitionBuffer(dst_buffer, CopyDst);

	vk::BufferCopy region{};
	region.setSrcOffset(src_offset)
	    .setDstOffset(dst_offset)
	    .setSize(size);

	current_command->getHandle().copyBuffer(
	    vk_src_buffer->getHandle(),
	    vk_dst_buffer->getHandle(),
	    region);
}

void VulkanCommandList::writeBuffer(RHIBuffer* buffer, uint64_t offset, const void* data, uint64_t size)
{
	endRenderPass();

	if (size == 0)
		return;

	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	assert(vk_buffer && "A buffer to write cannot be null.");

	RHIBufferDesc staging_desc{};
	staging_desc.setSize(size)
	    .setUsage(RHIBufferUsage::CopySrc)
	    .setAccess(RHIAccessMode::Write);

	auto staging_buffer = device.createBuffer(staging_desc);

	void* mapped = device.mapBuffer(staging_buffer.get(), RHIAccessMode::Write);
	std::memcpy(mapped, data, static_cast<size_t>(size));
	device.unmapBuffer(staging_buffer.get());

	copyBuffer(buffer, offset, staging_buffer.get(), 0, size);

	current_command->trackResource(std::move(staging_buffer));
}

void VulkanCommandList::pushConstants(const void* data, size_t size)
{
	assert(current_command && "Cannot push constants without an active command buffer.");

	current_command->getHandle().pushConstants(
	    current_layout,
	    current_visibility,
	    0,
	    static_cast<uint32_t>(size),
	    data);
}

void VulkanCommandList::draw(const RHIDrawArguments& args)
{
	assert(current_command && "Cannot draw without an active command buffer.");

	current_command->getHandle().draw(
	    args.vertex_count,
	    args.instance_count,
	    args.start_vertex,
	    args.start_instance);
}

void VulkanCommandList::drawIndexed(const RHIDrawArguments& args)
{
	assert(current_command && "Cannot draw indexed without an active command buffer.");

	current_command->getHandle().drawIndexed(
	    args.vertex_count,
	    args.instance_count,
	    args.start_index,
	    args.start_vertex,
	    args.start_instance);
}

void VulkanCommandList::transitionTexture(RHITexture* texture, RHIResourceState new_state)
{
	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	assert(vk_texture && "A texture to transition cannot be null.");
	assert(current_command && "Cannot transition a texture without an active command buffer.");

	auto old_info = getTextureTransition(vk_texture->state);
	auto new_info = getTextureTransition(new_state);
	if (vk_texture->getState() == new_state && vk_texture->getLayout() == new_info.layout)
		return;

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
	vk_texture->state = new_state;
}

void VulkanCommandList::transitionBuffer(RHIBuffer* buffer, RHIResourceState new_state)
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	assert(vk_buffer && "A buffer to transition cannot be null.");
	assert(current_command && "Cannot transition a buffer without an active command buffer.");

	const auto old_info = getBufferTransition(vk_buffer->state);
	const auto new_info = getBufferTransition(new_state);
	if (vk_buffer->getState() == new_state)
		return;

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
	vk_buffer->state = new_state;
}

void VulkanCommandList::setGraphicsState(const RHIGraphicsState& state)
{
	assert(current_command && "Cannot set graphics state without an active command buffer.");

	// transition framebuffer
	if (state.framebuffer && (!rendering || graphics_state.framebuffer != state.framebuffer)) {
		endRenderPass();

		auto* vk_framebuffer = static_cast<VulkanFrameBuffer*>(state.framebuffer);
		assert(vk_framebuffer && "A framebuffer to transition cannot be null.");

		for (const auto& attachment : vk_framebuffer->getDesc().color_attachments)
			if (attachment.texture)
				transitionTexture(attachment.texture, RenderTarget);

		const auto& depth_attachment = vk_framebuffer->getDesc().depth_attachment;
		if (depth_attachment.texture)
			transitionTexture(depth_attachment.texture, depth_attachment.read_only ? DepthRead : DepthWrite);

		beginRenderPass(*state.framebuffer);
	}

	// bind pipeline
	auto* vk_pipeline = static_cast<VulkanGraphicsPipeline*>(state.pipeline);
	assert(vk_pipeline && "A graphics pipeline in the graphics state cannot be null.");

	graphics_state = state;
	current_layout = vk_pipeline->getLayout();
	current_visibility = toVkShaderStageFlags(vk_pipeline->getShaderMask());

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
	for (auto* set : state.binding_sets)
		if (auto* vk_set = dynamic_cast<VulkanDescriptorSet*>(set))
			descriptor_sets.push_back(vk_set->getHandle());
	if (!descriptor_sets.empty())
		current_command->getHandle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, current_layout, 0, descriptor_sets, {});

	// bind vertex and index buffers
	for (const auto& binding : state.vertex_buffers) {
		if (auto* vertex_buffer = dynamic_cast<VulkanBuffer*>(binding.buffer)) {
			current_command->getHandle().bindVertexBuffers(binding.slot, vertex_buffer->getHandle(), {binding.offset});
		}
	}

	if (auto* index_buffer = dynamic_cast<VulkanBuffer*>(state.index_buffer.buffer)) {
		current_command->getHandle().bindIndexBuffer(index_buffer->getHandle(), state.index_buffer.offset, toVkIndexType(state.index_buffer.index_type));
	}
}

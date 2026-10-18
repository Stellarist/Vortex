module;

#include <vk_mem_alloc.h>

module Runtime.Vulkan;

import vulkan;

namespace Vortex {

RHIRef<RHIBuffer> VulkanDevice::createBuffer(const RHIBufferDesc& desc)
{
	validateRHIBufferDesc(desc);

	auto buffer = makeRHIRef<VulkanBuffer>(*this, desc);

	VkBufferCreateInfo buffer_info{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .size = desc.size,
	    .usage = static_cast<VkBufferUsageFlags>(toVkBufferUsageFlags(desc.usage)),
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo alloc_info{
	    .usage = VMA_MEMORY_USAGE_AUTO,
	    .requiredFlags = static_cast<VkMemoryPropertyFlags>(toVkMemoryPropertyFlags(desc.access)),
	};

	if (desc.access == RHIAccessMode::Write)
		alloc_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	else if (desc.access == RHIAccessMode::Read)
		alloc_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

	VkBuffer vk_buffer{};
	const VkResult result = vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &vk_buffer, &buffer->allocation, &buffer->allocation_info);
	CHECK(result == VK_SUCCESS, "Failed to allocate Vulkan buffer ({} bytes, result {})",
	    desc.size, static_cast<int32>(result));

	buffer->buffer = vk_buffer;
	return buffer;
}

RHIRef<RHIBufferView> VulkanDevice::createBufferView(const RHIBufferViewDesc& desc)
{
	auto normalized_desc = normalizeRHIBufferViewDesc(desc);
	auto view = makeRHIRef<VulkanBufferView>(*this, normalized_desc);

	if (normalized_desc.type == RHIBufferViewType::Typed) {
		auto* buffer = static_cast<VulkanBuffer*>(normalized_desc.buffer.get());
		vk::BufferViewCreateInfo view_info{};
		view_info.setBuffer(buffer->getHandle())
		    .setFormat(toVkFormat(normalized_desc.format))
		    .setOffset(normalized_desc.offset)
		    .setRange(normalized_desc.size);
		view->view = device.createBufferView(view_info);
	}

	return view;
}

void* VulkanDevice::mapBuffer(RHIBuffer* buffer, RHIAccessMode mode) const
{
	auto* vk_buffer = dynamic_cast<VulkanBuffer*>(buffer);
	CHECK(vk_buffer && !vk_buffer->mapped && mode != RHIAccessMode::None && vk_buffer->desc.access == mode,
	    "Vulkan mapBuffer access must match an unmapped CPU-accessible buffer");

	void* mapped_data{};
	const auto result = vmaMapMemory(allocator, vk_buffer->allocation, &mapped_data);
	CHECK(result == VK_SUCCESS, "Failed to map Vulkan buffer '{}' (result {})",
	    vk_buffer->getName(), static_cast<int32>(result));

	if (mode == RHIAccessMode::Read) {
		const auto invalidate_result = vmaInvalidateAllocation(allocator, vk_buffer->allocation, 0, VK_WHOLE_SIZE);
		if (invalidate_result != VK_SUCCESS)
			vmaUnmapMemory(allocator, vk_buffer->allocation);

		CHECK(invalidate_result == VK_SUCCESS, "Failed to invalidate Vulkan buffer '{}' (result {})",
		    vk_buffer->getName(), static_cast<int32>(invalidate_result));
	}

	vk_buffer->mapped = true;
	return mapped_data;
}

void VulkanDevice::unmapBuffer(RHIBuffer* buffer) const noexcept
{
	auto* vk_buffer = dynamic_cast<VulkanBuffer*>(buffer);
	if (!vk_buffer || !vk_buffer->mapped)
		return;

	vmaUnmapMemory(allocator, vk_buffer->allocation);
	vk_buffer->mapped = false;
}

void VulkanDevice::destroyBuffer(VulkanBuffer* buffer) noexcept
{
	unmapBuffer(buffer);
	vmaDestroyBuffer(allocator, buffer->buffer, buffer->allocation);
	buffer->buffer = vk::Buffer{};
	buffer->allocation = {};
	buffer->allocation_info = {};
}

void VulkanDevice::destroyBufferView(VulkanBufferView* view) noexcept
{
	if (view->view)
		device.destroyBufferView(view->view);
	view->view = vk::BufferView{};
}


void VulkanCommandList::useBuffer(RHIBuffer* buffer, RHIResourceState state, std::string_view operation)
{
	CHECK(buffer, "Buffer cannot be null");
	validateRHIBufferState(*buffer, state, operation);
	current_command->trackResource(buffer);
}

void VulkanCommandList::clearBuffer(RHIBuffer* buffer, uint32 value)
{
	endRenderPass();

	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	CHECK(vk_buffer, "A buffer to clear cannot be null");
	CHECK(recording && current_command, "Cannot clear a buffer without an active command buffer");

	CHECK(buffer->getDesc().size % 4 == 0 && buffer->getDesc().access == RHIAccessMode::None, "clearBuffer requires a GPU buffer with a DWORD-aligned size");
	useBuffer(buffer, TransferDest, "clearBuffer");

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
	CHECK(recording && current_command, "Cannot copy buffers without an active command buffer");

	validateRHIBufferCopy(dst_buffer, dst_offset, src_buffer, src_offset, size);
	useBuffer(src_buffer, TransferSource, "copyBuffer source");
	useBuffer(dst_buffer, TransferDest, "copyBuffer destination");

	vk::BufferCopy2 region{};
	region.setSrcOffset(src_offset)
	    .setDstOffset(dst_offset)
	    .setSize(size);

	vk::CopyBufferInfo2 copy_info{};
	copy_info.setSrcBuffer(vk_src_buffer->getHandle())
	    .setDstBuffer(vk_dst_buffer->getHandle())
	    .setRegions(region);
	current_command->getHandle().copyBuffer2(copy_info);
}

void VulkanCommandList::writeBuffer(RHIBuffer* buffer, uint64 offset, const void* data, uint64 size)
{
	CHECK(recording && current_command, "Vulkan command list is not open");
	endRenderPass();

	if (size == 0)
		return;

	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	CHECK(vk_buffer && data && buffer->getDesc().access == RHIAccessMode::None &&
	        offset <= buffer->getDesc().size && size <= buffer->getDesc().size - offset,
	    "Invalid GPU buffer upload range");

	RHIBufferDesc staging_desc{};
	staging_desc.setSize(size)
	    .setUsage(RHIBufferUsage::TransferSource)
	    .setAccess(RHIAccessMode::Write);

	auto staging_buffer = device.createBuffer(staging_desc);

	void* mapped = device.mapBuffer(staging_buffer.get(), RHIAccessMode::Write);
	std::memcpy(mapped, data, static_cast<size_t>(size));
	device.unmapBuffer(staging_buffer.get());

	transitionBuffer(staging_buffer.get(), Unknown, TransferSource);
	transitionBuffer(buffer, buffer->getState(), TransferDest);
	copyBuffer(buffer, offset, staging_buffer.get(), 0, size);

	current_command->trackResource(staging_buffer.get());
}

void VulkanCommandList::transitionBuffer(RHIBuffer* buffer, RHIResourceState before, RHIResourceState after)
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	CHECK(vk_buffer, "A buffer to transition cannot be null");
	CHECK(recording && current_command, "Cannot transition a buffer without an active command buffer");
	current_command->trackResource(buffer);
	validateRHIBufferTransition(*buffer, before, after);

	const auto old_info = getBufferTransition(before);
	const auto new_info = getBufferTransition(after);

	vk::BufferMemoryBarrier2 barrier{};
	barrier.setSrcStageMask(old_info.stage)
	    .setDstStageMask(new_info.stage)
	    .setSrcAccessMask(old_info.access)
	    .setDstAccessMask(new_info.access)
	    .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
	    .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
	    .setBuffer(vk_buffer->getHandle())
	    .setOffset(0)
	    .setSize(vk_buffer->getDesc().size);

	vk::DependencyInfo dependency{};
	dependency.setBufferMemoryBarriers(barrier);

	current_command->getHandle().pipelineBarrier2(dependency);
	vk_buffer->state = after;
}

}        // namespace Vortex

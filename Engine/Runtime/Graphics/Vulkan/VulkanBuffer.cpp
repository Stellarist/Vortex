module;

#include <vk_mem_alloc.h>

module Runtime.Graphics;

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

	VkBuffer       vk_buffer{};
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
		auto*                    buffer = static_cast<VulkanBuffer*>(normalized_desc.buffer.get());
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
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	if (!vk_buffer)
		return nullptr;

	void*      mapped_data{};
	const auto result = vmaMapMemory(allocator, vk_buffer->allocation, &mapped_data);
	CHECK(result == VK_SUCCESS, "Failed to map Vulkan buffer '{}' (result {})",
	    vk_buffer->getName(), static_cast<int32>(result));

	return mapped_data;
}

void VulkanDevice::unmapBuffer(RHIBuffer* buffer) const noexcept
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	if (vk_buffer)
		vmaUnmapMemory(allocator, vk_buffer->allocation);
}

void VulkanDevice::bindBufferMemory(RHIBuffer* buffer, uint64 offset) const noexcept
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	if (vk_buffer)
		vmaBindBufferMemory2(allocator, vk_buffer->allocation, offset, vk_buffer->buffer, nullptr);
}

void VulkanDevice::destroyBuffer(VulkanBuffer* buffer) noexcept
{
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

}        // namespace Vortex

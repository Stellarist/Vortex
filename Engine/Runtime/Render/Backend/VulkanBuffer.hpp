#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"

struct VulkanMemoryInfo {
	uint32_t size;
	uint32_t index;
};

class VulkanBuffer {
private:
	vk::Buffer       buffer;
	vk::DeviceSize   size;
	vk::DeviceMemory memory;

	void* data{};

	size_t mapped_size{};
	size_t mapped_offset{};
	bool   mapped{};

	VulkanContext* context{};

	void create(vk::BufferUsageFlags usage, size_t size);
	void allocate(vk::MemoryPropertyFlags properties);
	void bind(size_t bind_offset = 0);

	VulkanMemoryInfo queryMemoryInfo(vk::MemoryPropertyFlags properties) const;

public:
	VulkanBuffer(VulkanContext& context, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	~VulkanBuffer();

	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	VulkanBuffer(VulkanBuffer&&) noexcept;
	VulkanBuffer& operator=(VulkanBuffer&&);

	void map(size_t map_size, size_t map_offset = {});
	void unmap();

	void copyTo(vk::Buffer dst, size_t size, size_t src_offset = 0, size_t dst_offset = 0);
	void copyFrom(vk::Buffer src, size_t size, size_t src_offset = 0, size_t dst_offset = 0);
	void upload(const void* src, size_t src_size, size_t dst_offset = 0);

	static std::unique_ptr<VulkanBuffer> createStatic(VulkanContext& context, vk::BufferUsageFlags Usage, const void* src, size_t size);
	static std::unique_ptr<VulkanBuffer> createDynamic(VulkanContext& context, vk::BufferUsageFlags Usage, const void* src, size_t size);

	vk::Buffer        get() const;
	vk::DeviceSize    getSize() const;
	vk::DeviceAddress getAddress() const;
};

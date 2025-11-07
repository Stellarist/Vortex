#include "Buffer.hpp"

#include "Device.hpp"
#include "Command.hpp"

Buffer::Buffer(Context& context, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) :
    context(&context), size(size)
{
	create(usage, size);
	allocate(properties);
	bind();
}

Buffer::~Buffer()
{
	if (memory)
		context->getDevice().logical().freeMemory(memory);

	if (buffer)
		context->getDevice().logical().destroyBuffer(buffer);
}

Buffer::Buffer(Buffer&& other) noexcept :
    buffer(std::exchange(other.buffer, nullptr)),
    size(other.size),
    memory(std::exchange(other.memory, nullptr)),
    data(other.data),
    mapped_size(other.mapped_size),
    mapped_offset(other.mapped_offset),
    mapped(other.mapped),
    context(std::exchange(other.context, nullptr))
{}

Buffer& Buffer::operator=(Buffer&& other)
{
	if (this != &other) {
		if (memory)
			context->getDevice().logical().freeMemory(memory);

		if (buffer)
			context->getDevice().logical().destroyBuffer(buffer);

		buffer = std::exchange(other.buffer, nullptr);
		size = other.size;
		memory = std::exchange(other.memory, nullptr);
		data = other.data;
		mapped_size = other.mapped_size;
		mapped_offset = other.mapped_offset;
		mapped = other.mapped;
		context = std::exchange(other.context, nullptr);
	}

	return *this;
}

void Buffer::create(vk::BufferUsageFlags usage, size_t size)
{
	vk::BufferCreateInfo create_info{};
	create_info.setSize(size)
	    .setUsage(usage)
	    .setSharingMode(vk::SharingMode::eExclusive);

	buffer = context->getDevice().logical().createBuffer(create_info);
}

void Buffer::allocate(vk::MemoryPropertyFlags properties)
{
	MemoryInfo info = queryMemoryInfo(properties);

	vk::MemoryAllocateInfo allocate_info{};
	allocate_info.setAllocationSize(info.size)
	    .setMemoryTypeIndex(info.index);

	memory = context->getDevice().logical().allocateMemory(allocate_info);
}

void Buffer::bind(size_t bind_offset)
{
	context->getDevice().logical().bindBufferMemory(buffer, memory, bind_offset);
}

void Buffer::map(size_t map_size, size_t map_offset)
{
	mapped = true;
	data = context->getDevice().logical().mapMemory(memory, map_offset, map_size);
}

void Buffer::unmap()
{
	mapped = false;
	if (data) {
		context->getDevice().logical().unmapMemory(memory);
		data = nullptr;
	}
}

void Buffer::copyTo(vk::Buffer dst, size_t size, size_t src_offset, size_t dst_offset)
{
	context->execute([&](CommandBuffer command) {
		vk::BufferCopy copy_region{};
		copy_region.setSrcOffset(src_offset)
		    .setDstOffset(dst_offset)
		    .setSize(size);

		command.get().copyBuffer(buffer, dst, copy_region);
	});
}

void Buffer::copyFrom(vk::Buffer src, size_t size, size_t src_offset, size_t dst_offset)
{
	context->execute([&](CommandBuffer command) {
		vk::BufferCopy copy_region{};
		copy_region.setSrcOffset(src_offset)
		    .setDstOffset(dst_offset)
		    .setSize(size);

		command.get().copyBuffer(src, buffer, copy_region);
	});
}

void Buffer::upload(const void* src, size_t src_size, size_t dst_offset)
{
	if (!mapped)
		map(src_size, dst_offset);
	else if (mapped_size != src_size || mapped_offset != dst_offset) {
		unmap();
		map(src_size, dst_offset);
	}

	std::memcpy(data, src, src_size);
}

std::unique_ptr<Buffer> Buffer::createStatic(Context& context, vk::BufferUsageFlags Usage, const void* src, size_t size)
{
	if (!src || size == 0)
		return nullptr;

	auto host_buffer = std::make_unique<Buffer>(context, size,
	    vk::BufferUsageFlagBits::eTransferSrc,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	host_buffer->upload(src, size);

	auto device_buffer = std::make_unique<Buffer>(context, size,
	    Usage | vk::BufferUsageFlagBits::eTransferDst,
	    vk::MemoryPropertyFlagBits::eDeviceLocal);
	device_buffer->copyFrom(host_buffer->get(), size);

	return device_buffer;
}

std::unique_ptr<Buffer> Buffer::createDynamic(Context& context, vk::BufferUsageFlags Usage, const void* src, size_t size)
{
	if (!src || size == 0)
		return nullptr;

	auto buffer = std::make_unique<Buffer>(context, size, Usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	buffer->upload(src, size);

	return buffer;
}

vk::Buffer Buffer::get() const
{
	return buffer;
}

vk::DeviceSize Buffer::getSize() const
{
	return size;
}

vk::DeviceAddress Buffer::getAddress() const
{
	vk::BufferDeviceAddressInfo address_info{};
	address_info.setBuffer(buffer);

	return context->getDevice().logical().getBufferAddress(address_info);
}

MemoryInfo Buffer::queryMemoryInfo(vk::MemoryPropertyFlags prop_flags) const
{
	MemoryInfo info{};

	auto requirements = context->getDevice().logical().getBufferMemoryRequirements(buffer);
	info.size = requirements.size;

	auto properties = context->getDevice().physical().getMemoryProperties();
	for (auto i = 0; i < properties.memoryTypeCount; i++)
		if (requirements.memoryTypeBits & (1 << i) && (properties.memoryTypes[i].propertyFlags & prop_flags)) {
			info.index = i;
			break;
		}

	return info;
}

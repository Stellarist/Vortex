#include "Image.hpp"

#include "Device.hpp"
#include "Command.hpp"

Image::Image(Context& context, const uint8_t* data, uint32_t width, uint32_t height, vk::Format format) :
    context(&context), format(format), width(width), height(height), channels(4), data(reinterpret_cast<const void*>(data))
{
	createBuffer(width * height * 4);
	createImage(width, height, format, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
	allocateMemory();

	context.execute([&](CommandBuffer command) {
		transitionImageLayout(command.get(), image, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		copyBufferToImage(command.get(), buffer->get(), image, width, height);
		transitionImageLayout(command.get(), image, format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	});

	createImageView(format, vk::ImageAspectFlagBits::eColor);
}

Image::Image(Context& context, uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage) :
    context(&context), format(format), width(width), height(height)
{
	createImage(width, height, format, usage);
	allocateMemory();

	vk::ImageAspectFlags aspect = (format == vk::Format::eD32Sfloat || format == vk::Format::eD24UnormS8Uint) ?
	    vk::ImageAspectFlagBits::eDepth :
	    vk::ImageAspectFlagBits::eColor;

	createImageView(format, aspect);
}

Image::~Image()
{
	context->getDevice().logical().destroyImageView(view);
	context->getDevice().logical().freeMemory(memory);
	context->getDevice().logical().destroyImage(image);
}

void Image::createBuffer(uint32_t image_size)
{
	buffer = std::make_unique<Buffer>(
	    *context,
	    image_size,
	    vk::BufferUsageFlagBits::eTransferSrc,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	buffer->upload(data, image_size);
}

void Image::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage)
{
	vk::ImageCreateInfo create_info{};
	create_info.setImageType(vk::ImageType::e2D)
	    .setExtent({width, height, 1})
	    .setMipLevels(1)
	    .setArrayLayers(1)
	    .setFormat(format)
	    .setTiling(vk::ImageTiling::eOptimal)
	    .setInitialLayout(vk::ImageLayout::eUndefined)
	    .setUsage(usage)
	    .setSamples(vk::SampleCountFlagBits::e1)
	    .setSharingMode(vk::SharingMode::eExclusive);

	image = context->getDevice().logical().createImage(create_info);
}

void Image::allocateMemory()
{
	vk::MemoryRequirements requirements = context->getDevice().logical().getImageMemoryRequirements(image);

	auto index = queryMemoryType(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

	vk::MemoryAllocateInfo alloc_info{};
	alloc_info.setAllocationSize(requirements.size)
	    .setMemoryTypeIndex(index);

	memory = context->getDevice().logical().allocateMemory(alloc_info);
	context->getDevice().logical().bindImageMemory(image, memory, 0);
}

void Image::createImageView(vk::Format format, vk::ImageAspectFlags aspect_flags)
{
	vk::ImageSubresourceRange range{};
	vk::ComponentMapping      mapping{};
	range.setBaseMipLevel(0)
	    .setLevelCount(1)
	    .setBaseArrayLayer(0)
	    .setLayerCount(1)
	    .setAspectMask(aspect_flags);

	vk::ImageViewCreateInfo create_info{};
	create_info.setImage(image)
	    .setViewType(vk::ImageViewType::e2D)
	    .setFormat(format)
	    .setSubresourceRange(range)
	    .setComponents(mapping);

	view = context->getDevice().logical().createImageView(create_info);
}

void Image::copyBufferToImage(vk::CommandBuffer command, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
	vk::ImageSubresourceLayers layers{};
	layers.setAspectMask(vk::ImageAspectFlagBits::eColor)
	    .setMipLevel(0)
	    .setBaseArrayLayer(0)
	    .setLayerCount(1);

	vk::BufferImageCopy region{};
	region.setBufferOffset(0)
	    .setBufferRowLength(0)
	    .setBufferImageHeight(0)
	    .setImageSubresource(layers)
	    .setImageOffset({0, 0, 0})
	    .setImageExtent({width, height, 1});

	command.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

void Image::transitionImageLayout(vk::CommandBuffer command, vk::Image image, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout)
{
	vk::ImageSubresourceRange range{};
	range.setAspectMask(vk::ImageAspectFlagBits::eColor)
	    .setBaseMipLevel(0)
	    .setLevelCount(1)
	    .setBaseArrayLayer(0)
	    .setLayerCount(1);

	vk::ImageMemoryBarrier barrier{};
	barrier.setOldLayout(old_layout)
	    .setNewLayout(new_layout)
	    .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
	    .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
	    .setImage(image)
	    .setSubresourceRange(range);

	vk::PipelineStageFlags src_stage{};
	vk::PipelineStageFlags dst_stage{};
	if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.setSrcAccessMask({});
		barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	} else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	} else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eDepthAttachmentOptimal) {
		barrier.setSrcAccessMask({});
		barrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead);

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	} else
		throw std::invalid_argument("unsupported layout transition!");

	command.pipelineBarrier(
	    src_stage,
	    dst_stage,
	    {},
	    nullptr,
	    nullptr,
	    barrier);
}

vk::Image Image::get() const
{
	return image;
}

vk::ImageView Image::getView() const
{
	return view;
}

vk::Format Image::getFormat() const
{
	return format;
}

void Image::setSampler(Sampler& sampler)
{
	this->sampler = &sampler;
}

Sampler& Image::getSampler() const
{
	return *sampler;
}

uint32_t Image::queryMemoryType(uint32_t type, vk::MemoryPropertyFlags prop_flags) const
{
	auto property = context->getDevice().physical().getMemoryProperties();

	for (std::uint32_t i = 0; i < property.memoryTypeCount; i++)
		if ((1 << i) & type && property.memoryTypes[i].propertyFlags & prop_flags)
			return i;

	return 0;
}

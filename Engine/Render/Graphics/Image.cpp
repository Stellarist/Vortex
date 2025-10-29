#include "Image.hpp"

#include <stb_image.h>

Image::Image(Context& context, std::string_view file_path) :
    context(&context)
{
	readImage(file_path);
	createBuffer(width * height * 4);
	createImage(width, height);
	allocateMemory();
	context.execute([&](vk::CommandBuffer command) {
		transitionImageLayout(command, image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		copyBufferToImage(command, buffer->get(), image, width, height);
		transitionImageLayout(command, image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	});
	createImageView();
	freeImage();
}

Image::~Image()
{
	context->getLogicalDevice().destroyImageView(view);
	context->getLogicalDevice().freeMemory(memory);
	context->getLogicalDevice().destroyImage(image);
}

void Image::readImage(std::string_view file_path)
{
	data = stbi_load(file_path.data(), &width, &height, &channels, STBI_rgb_alpha);
	if (!data)
		throw std::runtime_error("failed to load texture image!");
}

void Image::freeImage()
{
	if (data) {
		stbi_image_free(data);
		data = nullptr;
	}
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

void Image::createImage(uint32_t width, uint32_t height)
{
	vk::ImageCreateInfo create_info{};
	create_info.setImageType(vk::ImageType::e2D)
	    .setExtent({width, height, 1})
	    .setMipLevels(1)
	    .setArrayLayers(1)
	    .setFormat(vk::Format::eR8G8B8A8Srgb)
	    .setTiling(vk::ImageTiling::eOptimal)
	    .setInitialLayout(vk::ImageLayout::eUndefined)
	    .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
	    .setSamples(vk::SampleCountFlagBits::e1)
	    .setSharingMode(vk::SharingMode::eExclusive);

	image = context->getLogicalDevice().createImage(create_info);
}

void Image::allocateMemory()
{
	vk::MemoryRequirements requirements = context->getLogicalDevice().getImageMemoryRequirements(image);

	auto index = queryMemoryType(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

	vk::MemoryAllocateInfo alloc_info{};
	alloc_info.setAllocationSize(requirements.size)
	    .setMemoryTypeIndex(index);

	memory = context->getLogicalDevice().allocateMemory(alloc_info);
	context->getLogicalDevice().bindImageMemory(image, memory, 0);
}

void Image::createImageView()
{
	vk::ImageSubresourceRange range{};
	vk::ComponentMapping      mapping{};
	range.setBaseMipLevel(0)
	    .setLevelCount(1)
	    .setBaseArrayLayer(0)
	    .setLayerCount(1)
	    .setAspectMask(vk::ImageAspectFlagBits::eColor);

	vk::ImageViewCreateInfo create_info{};
	create_info.setImage(image)
	    .setViewType(vk::ImageViewType::e2D)
	    .setFormat(vk::Format::eR8G8B8A8Srgb)
	    .setSubresourceRange(range)
	    .setComponents(mapping);

	view = context->getLogicalDevice().createImageView(create_info);
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

vk::Image Image::get() const
{
	return image;
}

vk::ImageView Image::getView() const
{
	return view;
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
	auto property = context->getPhysicalDevice().getMemoryProperties();

	for (std::uint32_t i = 0; i < property.memoryTypeCount; i++)
		if ((1 << i) & type && property.memoryTypes[i].propertyFlags & prop_flags)
			return i;

	return 0;
}

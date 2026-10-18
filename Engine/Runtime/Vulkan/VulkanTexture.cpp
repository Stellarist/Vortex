module;

#include <vk_mem_alloc.h>

module Runtime.Vulkan;

import vulkan;

namespace Vortex {

RHIRef<RHITexture> VulkanDevice::createTexture(const RHITextureDesc& desc)
{
	validateRHITextureDesc(desc);

	auto texture = makeRHIRef<VulkanTexture>(*this, desc);

	VkImageCreateInfo image_info{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    .flags = desc.dimension == RHITextureDimension::TextureCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u,
	    .imageType = static_cast<VkImageType>(toVkImageType(desc.dimension)),
	    .format = static_cast<VkFormat>(toVkFormat(desc.format)),
	    .extent = {desc.width, desc.height, desc.depth},
	    .mipLevels = desc.mip_levels,
	    .arrayLayers = desc.array_layers,
	    .samples = static_cast<VkSampleCountFlagBits>(toVkSampleCountFlagBits(desc.sample_count)),
	    .tiling = VK_IMAGE_TILING_OPTIMAL,
	    .usage = static_cast<VkImageUsageFlags>(toVkImageUsageFlags(desc.usage)),
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VmaAllocationCreateInfo alloc_info{
	    .usage = VMA_MEMORY_USAGE_AUTO,
	};

	VkImage vk_image{};
	const VkResult result = vmaCreateImage(allocator, &image_info, &alloc_info, &vk_image, &texture->allocation, &texture->allocation_info);
	CHECK(result == VK_SUCCESS, "Failed to allocate Vulkan image ({}x{}x{}, result {})",
	    desc.width, desc.height, desc.depth, static_cast<int32>(result));

	texture->image = vk_image;
	texture->layout = vk::ImageLayout::eUndefined;
	return texture;
}

RHIRef<RHITextureView> VulkanDevice::createTextureView(const RHITextureViewDesc& desc)
{
	auto normalized_desc = normalizeRHITextureViewDesc(desc);
	auto* texture = static_cast<VulkanTexture*>(normalized_desc.texture.get());
	const auto& subresource = normalized_desc.subresource;

	vk::ImageSubresourceRange range{};
	range.setAspectMask(getVkImageAspectFlags(normalized_desc.format))
	    .setBaseMipLevel(subresource.base_mip_level)
	    .setLevelCount(subresource.level_count)
	    .setBaseArrayLayer(subresource.base_array_layer)
	    .setLayerCount(subresource.layer_count);

	vk::ImageViewCreateInfo view_info{};
	view_info.setImage(texture->getHandle())
	    .setViewType(toVkImageViewType(normalized_desc.dimension))
	    .setFormat(toVkFormat(normalized_desc.format))
	    .setSubresourceRange(range);

	auto view = makeRHIRef<VulkanTextureView>(*this, normalized_desc);
	view->view = device.createImageView(view_info);
	return view;
}

RHIRef<RHIStagingTexture> VulkanDevice::createStagingTexture(const RHITextureDesc& desc, RHIAccessMode access)
{
	validateRHITextureDesc(desc);
	CHECK(desc.sample_count == 1 && !hasRHIStencil(desc.format), "Staging requires a single-sample, single-plane format");
	CHECK(access == RHIAccessMode::Read || access == RHIAccessMode::Write, "Staging requires CPU read or write access");
	auto staging = makeRHIRef<VulkanStagingTexture>(*this, desc, access);

	VkBufferCreateInfo buffer_info{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .size = getVkStagingTextureByteSize(desc),
	    .usage = static_cast<VkBufferUsageFlags>(
	        access == RHIAccessMode::Write ?
	            VK_BUFFER_USAGE_TRANSFER_SRC_BIT :
	            VK_BUFFER_USAGE_TRANSFER_DST_BIT),
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo alloc_info{
	    .flags = static_cast<VmaAllocationCreateFlags>(
	        access == RHIAccessMode::Write ?
	            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT :
	            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT),
	    .usage = VMA_MEMORY_USAGE_AUTO,
	    .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | (access == RHIAccessMode::Write ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0u),
	    .preferredFlags = access == RHIAccessMode::Read ? VK_MEMORY_PROPERTY_HOST_CACHED_BIT : 0u,
	};

	VkBuffer buffer{};
	const auto result = vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer, &staging->allocation, nullptr);
	CHECK(result == VK_SUCCESS, "Failed to allocate Vulkan staging buffer (result {})", static_cast<int32>(result));
	staging->buffer = buffer;
	return staging;
}

RHIRef<RHISampler> VulkanDevice::createSampler(const RHISamplerDesc& desc)
{
	validateRHISamplerDesc(desc);

	vk::SamplerCreateInfo sampler_info{};
	sampler_info.setMagFilter(desc.mag_filter ? vk::Filter::eLinear : vk::Filter::eNearest)
	    .setMinFilter(desc.min_filter ? vk::Filter::eLinear : vk::Filter::eNearest)
	    .setMipmapMode(desc.mip_filter ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest)
	    .setAddressModeU(toVkSamplerAddressMode(desc.address_u))
	    .setAddressModeV(toVkSamplerAddressMode(desc.address_v))
	    .setAddressModeW(toVkSamplerAddressMode(desc.address_w))
	    .setMipLodBias(desc.mip_bias)
	    .setMinLod(0.0f)
	    .setMaxLod(VK_LOD_CLAMP_NONE)
	    .setBorderColor(vk::BorderColor::eIntOpaqueBlack);

	auto sampler = makeRHIRef<VulkanSampler>(*this, desc);
	sampler->sampler = device.createSampler(sampler_info);
	return sampler;
}

void* VulkanDevice::mapStagingTexture(RHIStagingTexture* resource, RHIAccessMode mode,
    uint64& row_pitch, uint64& depth_pitch, uint32 mip, uint32 layer) const
{
	auto* staging = dynamic_cast<VulkanStagingTexture*>(resource);
	CHECK(staging && !staging->mapped && mode == staging->access && mip < staging->desc.mip_levels && layer < staging->desc.array_layers,
	    "Invalid Vulkan staging map access or subresource");

	void* data{};
	const auto result = vmaMapMemory(allocator, staging->allocation, &data);
	CHECK(result == VK_SUCCESS, "Failed to map Vulkan staging buffer (result {})", static_cast<int32>(result));

	if (mode == RHIAccessMode::Read) {
		const auto invalidate_result = vmaInvalidateAllocation(allocator, staging->allocation, 0, VK_WHOLE_SIZE);
		if (invalidate_result != VK_SUCCESS)
			vmaUnmapMemory(allocator, staging->allocation);
		CHECK(invalidate_result == VK_SUCCESS, "Failed to invalidate Vulkan staging buffer (result {})", static_cast<int32>(invalidate_result));
	}
	staging->mapped = true;
	row_pitch = uint64(std::max(1u, staging->desc.width >> mip)) * getRHIFormatByteSize(staging->desc.format);
	depth_pitch = row_pitch * std::max(1u, staging->desc.height >> mip);

	return static_cast<std::byte*>(data) + getVkStagingTextureSubresourceOffset(staging->desc, mip, layer);
}

void VulkanDevice::unmapStagingTexture(RHIStagingTexture* resource) const noexcept
{
	auto* staging = dynamic_cast<VulkanStagingTexture*>(resource);
	if (!staging || !staging->mapped)
		return;

	vmaUnmapMemory(allocator, staging->allocation);
	staging->mapped = false;
}

void VulkanDevice::destroyTexture(VulkanTexture* texture) noexcept
{
	vmaDestroyImage(allocator, texture->image, texture->allocation);
	texture->image = vk::Image{};
	texture->layout = vk::ImageLayout{};
	texture->allocation = {};
	texture->allocation_info = {};
}

void VulkanDevice::destroyTextureView(VulkanTextureView* view) noexcept
{
	if (view->view)
		device.destroyImageView(view->view);
	view->view = vk::ImageView{};
}

void VulkanDevice::destroyStagingTexture(VulkanStagingTexture* staging) noexcept
{
	unmapStagingTexture(staging);
	vmaDestroyBuffer(allocator, staging->buffer, staging->allocation);
	staging->buffer = vk::Buffer{};
	staging->allocation = {};
}

void VulkanDevice::destroySampler(VulkanSampler* sampler) noexcept
{
	if (sampler->sampler)
		device.destroySampler(sampler->sampler);
	sampler->sampler = vk::Sampler{};
}


void VulkanCommandList::clearTexture(RHITexture* texture, const RHIColor& color)
{
	endRenderPass();

	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	CHECK(vk_texture, "A texture to clear cannot be null");
	CHECK(recording && current_command, "Cannot clear a texture without an active command buffer");

	CHECK(!isRHIDepthFormat(texture->getDesc().format), "clearTexture requires a color texture");
	validateRHITextureState(*texture, TransferDest, "clearTexture");
	current_command->trackResource(texture);

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
	CHECK(recording && current_command, "Cannot clear a texture without an active command buffer");

	CHECK(isRHIDepthFormat(texture->getDesc().format) && (!clear_stencil || hasRHIStencil(texture->getDesc().format)),
	    "Invalid depth/stencil clear");
	validateRHITextureState(*texture, TransferDest, "clearDepthTexture");
	current_command->trackResource(texture);

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
	CHECK(recording && current_command,
	    "Cannot copy textures without an active command buffer");

	validateRHITextureState(*src_texture, TransferSource, "copyTexture source");
	validateRHITextureState(*dst_texture, TransferDest, "copyTexture destination");
	const auto normalized_src = normalizeRHITextureSlice(src_texture->getDesc(), src_slice);
	const auto normalized_dst = normalizeRHITextureSlice(dst_texture->getDesc(), dst_slice);

	CHECK(dst_texture != src_texture, "Texture copy requires different resources");
	validateRHITextureCopy(dst_texture->getDesc(), normalized_dst, src_texture->getDesc(), normalized_src);
	current_command->trackResource(src_texture);
	current_command->trackResource(dst_texture);

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
	    static_cast<uint32>(normalized_src.width),
	    static_cast<uint32>(normalized_src.height),
	    static_cast<uint32>(normalized_src.depth)};

	vk::ImageCopy2 region{};
	region.setSrcSubresource(src_subresource)
	    .setSrcOffset(vk::Offset3D(normalized_src.x, normalized_src.y, normalized_src.z))
	    .setDstSubresource(dst_subresource)
	    .setDstOffset(vk::Offset3D(normalized_dst.x, normalized_dst.y, normalized_dst.z))
	    .setExtent(mip_extent);

	vk::CopyImageInfo2 copy_info{};
	copy_info.setSrcImage(vk_src_texture->getHandle())
	    .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
	    .setDstImage(vk_dst_texture->getHandle())
	    .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
	    .setRegions(region);
	current_command->getHandle().copyImage2(copy_info);
}

void VulkanCommandList::copyTexture(RHIStagingTexture* dst_staging, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice)
{
	CHECK(recording && current_command, "Vulkan command list is not open");
	endRenderPass();

	auto* source = dynamic_cast<VulkanTexture*>(src_texture);
	auto* staging = dynamic_cast<VulkanStagingTexture*>(dst_staging);
	CHECK(source && staging && !staging->mapped && staging->access == RHIAccessMode::Read, "Invalid texture readback resources");
	const auto src = normalizeRHITextureSlice(source->getDesc(), src_slice);
	const auto dst = normalizeRHITextureSlice(staging->desc, dst_slice);
	validateRHITextureCopy(staging->desc, dst, source->getDesc(), src);
	validateRHITextureState(*source, TransferSource, "texture readback");

	const uint32 width = std::max(1u, staging->desc.width >> dst.mip_level);
	const uint32 height = std::max(1u, staging->desc.height >> dst.mip_level);
	const uint64 pixel_size = getRHIFormatByteSize(staging->desc.format);
	const uint64 offset = getVkStagingTextureSubresourceOffset(staging->desc, dst.mip_level, dst.array_layer) +
	    ((uint64(dst.z) * height + dst.y) * width + dst.x) * pixel_size;

	vk::ImageSubresourceLayers src_subresource{};
	src_subresource.setAspectMask(getVkImageAspectFlags(source->getDesc().format))
	    .setMipLevel(src.mip_level)
	    .setBaseArrayLayer(src.array_layer)
	    .setLayerCount(1);

	vk::BufferImageCopy2 region{};
	region.setBufferOffset(offset)
	    .setBufferRowLength(width)
	    .setBufferImageHeight(height)
	    .setImageSubresource(src_subresource)
	    .setImageOffset({src.x, src.y, src.z})
	    .setImageExtent({uint32(src.width), uint32(src.height), uint32(src.depth)});
	vk::CopyImageToBufferInfo2 copy_info{};
	copy_info.setSrcImage(source->getHandle())
	    .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
	    .setDstBuffer(staging->getHandle())
	    .setRegions(region);
	current_command->getHandle().copyImageToBuffer2(copy_info);

	vk::BufferMemoryBarrier2 barrier{};
	barrier.setSrcStageMask(vk::PipelineStageFlagBits2::eCopy)
	    .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
	    .setDstStageMask(vk::PipelineStageFlagBits2::eCopy | vk::PipelineStageFlagBits2::eHost)
	    .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eHostRead)
	    .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
	    .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
	    .setBuffer(staging->getHandle())
	    .setSize(getVkStagingTextureByteSize(staging->desc));

	vk::DependencyInfo dependency{};
	dependency.setBufferMemoryBarriers(barrier);
	current_command->getHandle().pipelineBarrier2(dependency);
	current_command->trackResource(source);
	current_command->trackResource(staging);
}

void VulkanCommandList::copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHIStagingTexture* src_staging, const RHITextureSlice& src_slice)
{
	CHECK(recording && current_command, "Vulkan command list is not open");
	endRenderPass();

	auto* destination = dynamic_cast<VulkanTexture*>(dst_texture);
	auto* staging = dynamic_cast<VulkanStagingTexture*>(src_staging);
	CHECK(destination && staging && !staging->mapped && staging->access == RHIAccessMode::Write, "Invalid texture upload resources");
	const auto src = normalizeRHITextureSlice(staging->desc, src_slice);
	const auto dst = normalizeRHITextureSlice(destination->getDesc(), dst_slice);
	validateRHITextureCopy(destination->getDesc(), dst, staging->desc, src);
	validateRHITextureState(*destination, TransferDest, "texture upload");

	const uint32 width = std::max(1u, staging->desc.width >> src.mip_level);
	const uint32 height = std::max(1u, staging->desc.height >> src.mip_level);
	const uint64 pixel_size = getRHIFormatByteSize(staging->desc.format);
	const uint64 offset = getVkStagingTextureSubresourceOffset(staging->desc, src.mip_level, src.array_layer) +
	    ((uint64(src.z) * height + src.y) * width + src.x) * pixel_size;

	vk::ImageSubresourceLayers dst_subresource{};
	dst_subresource.setAspectMask(getVkImageAspectFlags(destination->getDesc().format))
	    .setMipLevel(dst.mip_level)
	    .setBaseArrayLayer(dst.array_layer)
	    .setLayerCount(1);

	vk::BufferImageCopy2 region{};
	region.setBufferOffset(offset)
	    .setBufferRowLength(width)
	    .setBufferImageHeight(height)
	    .setImageSubresource(dst_subresource)
	    .setImageOffset({dst.x, dst.y, dst.z})
	    .setImageExtent({uint32(dst.width), uint32(dst.height), uint32(dst.depth)});

	vk::CopyBufferToImageInfo2 copy_info{};
	copy_info.setSrcBuffer(staging->getHandle())
	    .setDstImage(destination->getHandle())
	    .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
	    .setRegions(region);
	current_command->getHandle().copyBufferToImage2(copy_info);
	current_command->trackResource(destination);
	current_command->trackResource(staging);
}

void VulkanCommandList::writeTexture(RHITexture* texture, const RHITextureSlice& slice, const void* data, uint64 size)
{
	CHECK(recording && current_command, "Vulkan command list is not open");
	endRenderPass();
	if (size == 0)
		return;

	CHECK(texture && data, "Texture upload requires a texture and data");
	const auto& texture_desc = texture->getDesc();
	const auto region = normalizeRHITextureSlice(texture_desc, slice);
	const uint64 row_size = uint64(region.width) * getRHIFormatByteSize(texture_desc.format);
	CHECK(size >= row_size * region.height * region.depth, "Texture upload data is smaller than the requested slice");

	RHITextureDesc desc{};
	desc.setWidth(region.width)
	    .setHeight(region.height)
	    .setDepth(region.depth)
	    .setFormat(texture_desc.format)
	    .setDimension(texture_desc.dimension == RHITextureDimension::TextureCube ? RHITextureDimension::Texture2D : texture_desc.dimension)
	    .setUsage(RHITextureUsage::TransferSource);
	if (isRHIDepthFormat(desc.format))
		desc.usage |= RHITextureUsage::DepthStencil;

	uint64 row_pitch{}, depth_pitch{};
	auto staging = device.createStagingTexture(desc, RHIAccessMode::Write);
	auto* mapped = static_cast<std::byte*>(device.mapStagingTexture(staging.get(), RHIAccessMode::Write, row_pitch, depth_pitch));
	for (int z = 0; z < region.depth; ++z)
		for (int y = 0; y < region.height; ++y)
			std::memcpy(mapped + z * depth_pitch + y * row_pitch, static_cast<const std::byte*>(data) + (uint64(z) * region.height + y) * row_size, row_size);

	device.unmapStagingTexture(staging.get());
	transitionTexture(texture, texture->getState(), TransferDest);
	copyTexture(texture, region, staging.get(), {});
}

void VulkanCommandList::transitionTexture(RHITexture* texture, RHIResourceState before, RHIResourceState after)
{
	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	CHECK(vk_texture, "A texture to transition cannot be null");
	CHECK(recording && current_command, "Cannot transition a texture without an active command buffer");
	current_command->trackResource(texture);
	CHECK(before == texture->getState() && after != Unknown, "Invalid texture transition state");

	auto old_info = getTextureTransition(before);
	auto new_info = getTextureTransition(after);

	vk::ImageMemoryBarrier2 barrier{};
	barrier.setOldLayout(vk_texture->layout)
	    .setNewLayout(new_info.layout)
	    .setSrcStageMask(old_info.stage)
	    .setDstStageMask(new_info.stage)
	    .setSrcAccessMask(old_info.access)
	    .setDstAccessMask(new_info.access)
	    .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
	    .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
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

}        // namespace Vortex

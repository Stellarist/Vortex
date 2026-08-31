module;

#include <vk_mem_alloc.h>

module Runtime.Graphics;

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

	VkImage        vk_image{};
	const VkResult result = vmaCreateImage(allocator, &image_info, &alloc_info, &vk_image, &texture->allocation, &texture->allocation_info);
	CHECK(result == VK_SUCCESS, "Failed to allocate Vulkan image ({}x{}x{}, result {})",
	    desc.width, desc.height, desc.depth, static_cast<int32>(result));

	texture->image = vk_image;
	texture->layout = vk::ImageLayout::eUndefined;
	return texture;
}

RHIRef<RHITextureView> VulkanDevice::createTextureView(const RHITextureViewDesc& desc)
{
	auto        normalized_desc = normalizeRHITextureViewDesc(desc);
	auto*       texture = static_cast<VulkanTexture*>(normalized_desc.texture.get());
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

RHIRef<RHIStagingTexture> VulkanDevice::createStagingTexture(const RHITextureDesc& desc)
{
	validateRHITextureDesc(desc);
	auto staging = makeRHIRef<VulkanStagingTexture>(*this, desc);

	VkBufferCreateInfo buffer_info{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .size = getVkFormatByteSize(desc.format) * desc.width * desc.height * desc.depth * desc.array_layers,
	    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo alloc_info{
	    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
	    .usage = VMA_MEMORY_USAGE_AUTO,
	    .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	};

	VkBuffer       vk_buffer{};
	const VkResult result = vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &vk_buffer, &staging->allocation, &staging->allocation_info);
	CHECK(result == VK_SUCCESS, "Failed to allocate Vulkan staging buffer for {}x{}x{} texture (result {})",
	    desc.width, desc.height, desc.depth, static_cast<int32>(result));

	staging->buffer = vk_buffer;
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

void* VulkanDevice::mapStagingTexture(RHIStagingTexture* staging_texture, RHIAccessMode mode) const
{
	auto* staging = static_cast<VulkanStagingTexture*>(staging_texture);
	if (!staging)
		return nullptr;

	void*      mapped_data{};
	const auto result = vmaMapMemory(allocator, staging->allocation, &mapped_data);
	CHECK(result == VK_SUCCESS, "Failed to map Vulkan staging texture (result {})",
	    static_cast<int32>(result));

	return mapped_data;
}

void VulkanDevice::unmapStagingTexture(RHIStagingTexture* staging_texture) const noexcept
{
	auto* staging = static_cast<VulkanStagingTexture*>(staging_texture);
	if (staging)
		vmaUnmapMemory(allocator, staging->allocation);
}

void VulkanDevice::bindTextureMemory(RHITexture* texture, uint64 offset) const noexcept
{
	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	if (vk_texture)
		vmaBindImageMemory2(allocator, vk_texture->allocation, offset, vk_texture->image, nullptr);
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

void VulkanDevice::destroyStagingTexture(VulkanStagingTexture* staging_texture) noexcept
{
	vmaDestroyBuffer(allocator, staging_texture->buffer, staging_texture->allocation);
	staging_texture->buffer = vk::Buffer{};
	staging_texture->allocation = {};
	staging_texture->allocation_info = {};
}

void VulkanDevice::destroySampler(VulkanSampler* sampler) noexcept
{
	if (sampler->sampler)
		device.destroySampler(sampler->sampler);
	sampler->sampler = vk::Sampler{};
}

}        // namespace Vortex

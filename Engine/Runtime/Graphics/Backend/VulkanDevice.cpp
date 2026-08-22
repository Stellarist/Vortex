module;

#define VMA_IMPLEMENTATION

#include <cassert>
#include <vk_mem_alloc.h>

module Runtime.Graphics;

import vulkan;

namespace Vortex {

VulkanDevice::VulkanDevice(VulkanContext& context, vk::Device device) :
    device(device), context(&context)
{
	VmaAllocatorCreateInfo create_info{
	    .physicalDevice = context.getPhysicalDevice(),
	    .device = device,
	    .instance = context.getInstance(),
	    .vulkanApiVersion = VK_API_VERSION_1_4,
	};

	if (vmaCreateAllocator(&create_info, &allocator) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan memory allocator");

	queue = std::make_unique<VulkanQueue>(*this, RHICommandQueue::Graphics, context.getQueueIndices().graphics_family.value());
}

VulkanDevice::~VulkanDevice() noexcept
{
	device.waitIdle();
	queue.reset();

	vmaDestroyAllocator(allocator);

	device.destroy();
	device = vk::Device{};
}

RHIRef<RHIBuffer> VulkanDevice::createBuffer(const RHIBufferDesc& desc)
{
	assert(desc.size > 0 && desc.usage != RHIBufferUsage::None && "A buffer requires a non-zero size and at least one usage flag.");

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
	VkResult result = vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &vk_buffer, &buffer->allocation, &buffer->allocation_info);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate Vulkan buffer.");

	buffer->buffer = vk_buffer;

	return buffer;
}

RHIRef<RHIBufferView> VulkanDevice::createBufferView(const RHIBufferViewDesc& desc)
{
	assert(desc.buffer && "Cannot create a buffer view without a buffer.");

	const auto& buffer_desc = desc.buffer->getDesc();
	assert(desc.offset < buffer_desc.size && "Buffer view offset is outside the buffer.");

	auto normalized_desc = desc;
	normalized_desc.size = desc.size == 0 ? buffer_desc.size - desc.offset : desc.size;
	assert(normalized_desc.size <= buffer_desc.size - desc.offset && "Buffer view range is outside the buffer.");

	switch (desc.type) {
	case RHIBufferViewType::Constant:
		assert((buffer_desc.usage & RHIBufferUsage::ConstantBuffer) != RHIBufferUsage::None &&
		    "A constant buffer view requires ConstantBuffer usage.");
		break;

	case RHIBufferViewType::Structured:
		normalized_desc.stride = desc.stride == 0 ? buffer_desc.stride : desc.stride;
		assert((buffer_desc.usage & RHIBufferUsage::StorageBuffer) != RHIBufferUsage::None && normalized_desc.stride > 0 &&
		    "A structured buffer view requires StorageBuffer usage and a non-zero stride.");
		assert((normalized_desc.offset % normalized_desc.stride) == 0 && (normalized_desc.size % normalized_desc.stride) == 0 &&
		    "A structured buffer view range must be aligned to its stride.");
		break;

	case RHIBufferViewType::Typed:
		assert((buffer_desc.usage & RHIBufferUsage::TypedBuffer) != RHIBufferUsage::None && desc.format != RHIFormat::Unknown &&
		    "A typed buffer view requires TypedBuffer usage and a concrete format.");
		{
			const auto format_size = getVkFormatByteSize(desc.format);
			assert(format_size > 0 && (normalized_desc.offset % format_size) == 0 && (normalized_desc.size % format_size) == 0 &&
			    "A typed buffer view range must be aligned to its format size.");
		}
		break;

	case RHIBufferViewType::Raw:
		assert((buffer_desc.usage & RHIBufferUsage::StorageBuffer) != RHIBufferUsage::None && (desc.offset % 4) == 0 &&
		    (normalized_desc.size % 4) == 0 && "A raw buffer view requires StorageBuffer usage and a 4-byte aligned range.");
		break;
	}

	auto view = makeRHIRef<VulkanBufferView>(*this, normalized_desc);
	if (desc.type == RHIBufferViewType::Typed) {
		auto* buffer = static_cast<VulkanBuffer*>(desc.buffer.get());

		vk::BufferViewCreateInfo view_info{};
		view_info.setBuffer(buffer->getHandle())
		    .setFormat(toVkFormat(desc.format))
		    .setOffset(normalized_desc.offset)
		    .setRange(normalized_desc.size);
		view->view = device.createBufferView(view_info);
	}

	return view;
}

RHIRef<RHITexture> VulkanDevice::createTexture(const RHITextureDesc& desc)
{
	assert(desc.width > 0 && desc.height > 0 && desc.depth > 0 && desc.mip_levels > 0 && desc.array_layers > 0 &&
	    "A texture requires non-zero dimensions, mip levels and array layers.");
	assert(desc.format != RHIFormat::Unknown && desc.usage != RHITextureUsage::None &&
	    "A texture requires a concrete format and at least one usage flag.");
	assert((desc.dimension != RHITextureDimension::Texture1D || (desc.height == 1 && desc.depth == 1)) &&
	    "A 1D texture requires height and depth equal to one.");
	assert((desc.dimension != RHITextureDimension::Texture2D || desc.depth == 1) &&
	    "A 2D texture requires depth equal to one.");
	assert((desc.dimension != RHITextureDimension::Texture3D || desc.array_layers == 1) &&
	    "A 3D texture cannot have array layers.");
	assert((desc.dimension != RHITextureDimension::TextureCube || (desc.array_layers >= 6 && (desc.array_layers % 6) == 0)) &&
	    "A cube texture requires an array layer count that is a multiple of six.");

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

	VkImage  vk_image{};
	VkResult result = vmaCreateImage(allocator, &image_info, &alloc_info, &vk_image, &texture->allocation, &texture->allocation_info);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate Vulkan image.");

	texture->image = vk_image;
	texture->layout = vk::ImageLayout::eUndefined;

	return texture;
}

RHIRef<RHITextureView> VulkanDevice::createTextureView(const RHITextureViewDesc& desc)
{
	assert(desc.texture && "Cannot create a texture view without a texture.");

	const auto& texture_desc = desc.texture->getDesc();
	auto        normalized_desc = desc;
	normalized_desc.format = desc.format == RHIFormat::Unknown ? texture_desc.format : desc.format;

	if (normalized_desc.format != texture_desc.format)
		throw std::runtime_error("Texture format reinterpretation is not supported yet.");

	if (desc.dimension == RHITextureViewDimension::Automatic) {
		switch (texture_desc.dimension) {
		case RHITextureDimension::Texture1D:
			normalized_desc.dimension = texture_desc.array_layers > 1 ? RHITextureViewDimension::Texture1DArray : RHITextureViewDimension::Texture1D;
			break;

		case RHITextureDimension::Texture2D:
			normalized_desc.dimension = texture_desc.array_layers > 1 ? RHITextureViewDimension::Texture2DArray : RHITextureViewDimension::Texture2D;
			break;

		case RHITextureDimension::Texture3D:
			normalized_desc.dimension = RHITextureViewDimension::Texture3D;
			break;

		case RHITextureDimension::TextureCube:
			normalized_desc.dimension = texture_desc.array_layers > 6 ?
			    RHITextureViewDimension::TextureCubeArray :
			    RHITextureViewDimension::TextureCube;
			break;
		}
	}

	bool dimension_compatible{};
	switch (texture_desc.dimension) {
	case RHITextureDimension::Texture1D:
		dimension_compatible = normalized_desc.dimension == RHITextureViewDimension::Texture1D ||
		    normalized_desc.dimension == RHITextureViewDimension::Texture1DArray;
		break;

	case RHITextureDimension::Texture2D:
		dimension_compatible = normalized_desc.dimension == RHITextureViewDimension::Texture2D ||
		    normalized_desc.dimension == RHITextureViewDimension::Texture2DArray;
		break;

	case RHITextureDimension::Texture3D:
		dimension_compatible = normalized_desc.dimension == RHITextureViewDimension::Texture3D;
		break;

	case RHITextureDimension::TextureCube:
		dimension_compatible = normalized_desc.dimension == RHITextureViewDimension::Texture2D ||
		    normalized_desc.dimension == RHITextureViewDimension::Texture2DArray ||
		    normalized_desc.dimension == RHITextureViewDimension::TextureCube ||
		    normalized_desc.dimension == RHITextureViewDimension::TextureCubeArray;
		break;
	}
	assert(dimension_compatible && "Texture view dimension is incompatible with the texture dimension.");

	auto& subresource = normalized_desc.subresource;
	assert(subresource.base_mip_level < texture_desc.mip_levels && subresource.base_array_layer < texture_desc.array_layers &&
	    "Texture view subresource starts outside the texture.");

	subresource.level_count = subresource.level_count == 0 ? texture_desc.mip_levels - subresource.base_mip_level : subresource.level_count;
	subresource.layer_count = subresource.layer_count == 0 ? texture_desc.array_layers - subresource.base_array_layer : subresource.layer_count;
	assert(subresource.level_count <= texture_desc.mip_levels - subresource.base_mip_level &&
	    subresource.layer_count <= texture_desc.array_layers - subresource.base_array_layer &&
	    "Texture view subresource range is outside the texture.");

	assert(((normalized_desc.dimension != RHITextureViewDimension::Texture1D && normalized_desc.dimension != RHITextureViewDimension::Texture2D &&
	            normalized_desc.dimension != RHITextureViewDimension::Texture3D) ||
	           subresource.layer_count == 1) &&
	    "A non-array texture view must select exactly one array layer.");

	assert((normalized_desc.dimension != RHITextureViewDimension::TextureCube || subresource.layer_count == 6) &&
	    "A cube texture view must select exactly six array layers.");

	assert(((normalized_desc.dimension != RHITextureViewDimension::TextureCube &&
	            normalized_desc.dimension != RHITextureViewDimension::TextureCubeArray) ||
	           ((subresource.base_array_layer % 6) == 0 && (subresource.layer_count % 6) == 0)) &&
	    "Cube texture views require six-layer aligned subresources.");

	const auto usage = texture_desc.usage;
	assert((desc.type != RHITextureViewType::ShaderResource || (usage & RHITextureUsage::Sampled) != RHITextureUsage::None) &&
	    "A shader resource view requires Sampled texture usage.");
	assert((desc.type != RHITextureViewType::UnorderedAccess || (usage & RHITextureUsage::Storage) != RHITextureUsage::None) &&
	    "An unordered access view requires Storage texture usage.");
	assert((desc.type != RHITextureViewType::RenderTarget || (usage & RHITextureUsage::RenderTarget) != RHITextureUsage::None) &&
	    "A render target view requires RenderTarget texture usage.");
	assert((desc.type != RHITextureViewType::DepthStencil || (usage & RHITextureUsage::DepthStencil) != RHITextureUsage::None) &&
	    "A depth stencil view requires DepthStencil texture usage.");

	auto* texture = static_cast<VulkanTexture*>(desc.texture.get());

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
	assert(desc.width > 0 && desc.height > 0 && desc.depth > 0 && "Cannot create an empty staging texture.");

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

	VkBuffer vk_buffer{};
	VkResult result = vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &vk_buffer, &staging->allocation, &staging->allocation_info);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate Vulkan staging buffer.");

	staging->buffer = vk_buffer;

	return staging;
}

RHIRef<RHISampler> VulkanDevice::createSampler(const RHISamplerDesc& desc)
{
	vk::SamplerCreateInfo sampler_info{};
	sampler_info.setMagFilter(desc.mag_filter ? vk::Filter::eLinear : vk::Filter::eNearest)
	    .setMinFilter(desc.min_filter ? vk::Filter::eLinear : vk::Filter::eNearest)
	    .setMipmapMode(desc.mip_filter ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest)
	    .setAddressModeU(toVkSamplerAddressMode(desc.address_u))
	    .setAddressModeV(toVkSamplerAddressMode(desc.address_v))
	    .setAddressModeW(toVkSamplerAddressMode(desc.address_w))
	    .setAnisotropyEnable(desc.max_anisotropy > 1.0f)
	    .setMaxAnisotropy(std::max(1.0f, desc.max_anisotropy))
	    .setMipLodBias(desc.mip_bias)
	    .setMinLod(0.0f)
	    .setMaxLod(VK_LOD_CLAMP_NONE)
	    .setBorderColor(vk::BorderColor::eIntOpaqueBlack);

	auto sampler = makeRHIRef<VulkanSampler>(*this, desc);
	sampler->sampler = device.createSampler(sampler_info);

	return sampler;
}

RHIRef<RHIShader> VulkanDevice::createShader(const RHIShaderDesc& desc, std::span<const std::byte> bytecode)
{
	assert(!bytecode.empty() && (bytecode.size() % sizeof(uint32)) == 0 && "Shader bytecode must contain non-empty, 4-byte aligned SPIR-V data.");
	vk::ShaderModuleCreateInfo shader_info{};
	shader_info.setCodeSize(bytecode.size())
	    .setPCode(reinterpret_cast<const uint32*>(bytecode.data()));

	auto shader = makeRHIRef<VulkanShader>(*this, desc);
	shader->shader = device.createShaderModule(shader_info);
	shader->stage_flags = toVkShaderStageFlagBits(desc.type);

	return shader;
}

RHIRef<RHIFramebuffer> VulkanDevice::createFramebuffer(const RHIFramebufferDesc& desc)
{
	auto framebuffer = makeRHIRef<VulkanFramebuffer>(*this, desc);

	for (const auto& attachment : desc.color_attachments) {
		auto* texture_view = static_cast<VulkanTextureView*>(attachment.texture_view.get());
		assert(texture_view && "A framebuffer color attachment cannot be null.");
		assert(texture_view->getDesc().type == RHITextureViewType::RenderTarget && "A framebuffer color attachment requires a render target view.");

		framebuffer->color_attachments_info.emplace_back()
		    .setImageView(texture_view->getHandle())
		    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
		    .setLoadOp(vk::AttachmentLoadOp::eLoad)
		    .setStoreOp(vk::AttachmentStoreOp::eStore);
	}

	if (desc.depth_attachment.texture_view) {
		auto* texture_view = static_cast<VulkanTextureView*>(desc.depth_attachment.texture_view.get());
		assert(texture_view && "A framebuffer depth attachment cannot be null.");
		assert(texture_view->getDesc().type == RHITextureViewType::DepthStencil && "A framebuffer depth attachment requires a depth stencil view.");
		auto& texture = texture_view->getTexture();

		auto layout = desc.depth_attachment.read_only ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eDepthStencilAttachmentOptimal;
		framebuffer->depth_attachment_info.setImageView(texture_view->getHandle())
		    .setImageLayout(layout)
		    .setLoadOp(vk::AttachmentLoadOp::eLoad)
		    .setStoreOp(vk::AttachmentStoreOp::eStore);

		if (getVkImageAspectFlags(texture.getDesc().format) & vk::ImageAspectFlagBits::eStencil)
			framebuffer->stencil_attachment_info.setImageView(texture_view->getHandle())
			    .setImageLayout(layout)
			    .setLoadOp(vk::AttachmentLoadOp::eLoad)
			    .setStoreOp(vk::AttachmentStoreOp::eStore);
	}

	return framebuffer;
}

RHIRef<RHIInputLayout> VulkanDevice::createInputLayout(const RHIInputLayoutDesc& desc)
{
	auto layout = makeRHIRef<VulkanInputLayout>(*this, desc);
	layout->desc = desc;

	vk::VertexInputBindingDescription binding{};
	for (const auto& desc : layout->desc.binding_descs) {
		binding.setBinding(desc.binding)
		    .setStride(desc.stride)
		    .setInputRate(desc.instance ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex);
		layout->bindings.push_back(binding);
	}

	layout->attributes.reserve(desc.attribute_descs.size());
	for (const auto& desc : layout->desc.attribute_descs) {
		vk::VertexInputAttributeDescription attribute{};
		attribute.setLocation(desc.location)
		    .setBinding(desc.binding)
		    .setFormat(toVkFormat(desc.format))
		    .setOffset(desc.offset);
		layout->attributes.push_back(attribute);
	}

	return layout;
}

RHIRef<RHIBindingLayout> VulkanDevice::createBindingLayout(const RHIBindingLayoutDesc& desc)
{
	assert(desc.visibility != RHIShaderType::None && "A binding layout requires shader visibility.");

	auto layout = makeRHIRef<VulkanBindingLayout>(*this, desc);
	auto shader_stages = toVkShaderStageFlags(desc.visibility);

	for (const auto& item : desc.bindings) {
		assert(item.type != RHIBindingType::None && "A binding layout item requires a concrete type.");

		if (item.type == RHIBindingType::PushConstants) {
			assert(item.size > 0 && item.size <= 128 && (item.size % 4) == 0 && "Push constants require a 4-byte aligned size up to 128 bytes.");
			continue;
		}

		if (item.count != 1)
			throw std::runtime_error("Binding arrays are not implemented yet; binding count must be one.");

		vk::DescriptorSetLayoutBinding layout_binding{};
		layout_binding.setBinding(item.slot)
		    .setDescriptorCount(item.count)
		    .setDescriptorType(toVkDescriptorType(item.type))
		    .setStageFlags(shader_stages);

		layout->bindings.push_back(layout_binding);
		if (auto it = std::find_if(layout->pool_sizes.begin(),
		        layout->pool_sizes.end(),
		        [&](const vk::DescriptorPoolSize& pool_size) {
			        return pool_size.type == layout_binding.descriptorType;
		        });
		    it != layout->pool_sizes.end())
			it->descriptorCount += item.count;
		else
			layout->pool_sizes.push_back({layout_binding.descriptorType, item.count});
	}

	vk::DescriptorSetLayoutCreateInfo layout_info{};
	layout_info.setBindings(layout->bindings);

	layout->layout = device.createDescriptorSetLayout(layout_info);

	return layout;
}

// todo: move descriptor set creation to a separate function
RHIRef<RHIBindingSet> VulkanDevice::createBindingSet(const RHIBindingSetDesc& desc, const RHIBindingLayout& layout)
{
	auto set = makeRHIRef<VulkanBindingSet>(*this, desc, layout);

	auto* vk_layout = static_cast<const VulkanBindingLayout*>(&layout);
	auto  vk_desc_layout = vk_layout->getHandle();

	vk::DescriptorPoolCreateInfo pool_info{};
	pool_info.setMaxSets(1)
	    .setPoolSizes(vk_layout->getPoolSizes());
	set->pool = device.createDescriptorPool(pool_info);

	vk::DescriptorSetAllocateInfo alloc_info{};
	alloc_info.setDescriptorPool(set->pool)
	    .setSetLayouts(vk_desc_layout);
	set->set = device.allocateDescriptorSets(alloc_info).front();

	writeBindingSet(set.get(), desc);

	return set;
}

RHIRef<RHICommandList> VulkanDevice::createCommandList(const RHICommandListDesc& desc)
{
	if (desc.queue_type != RHICommandQueue::Graphics)
		throw std::runtime_error("Only the graphics Vulkan queue is currently initialized.");

	auto layout = makeRHIRef<VulkanCommandList>(*this, desc);

	return layout;
}

RHIRef<RHIGraphicsPipeline> VulkanDevice::createGraphicsPipeline(const RHIGraphicsPipelineDesc& desc, const RHIFramebuffer& framebuffer)
{
	auto vk_pipeline = makeRHIRef<VulkanGraphicsPipeline>(*this, desc);

	auto  shader_mask = RHIShaderType::None;
	auto  shader_stages = std::vector<vk::PipelineShaderStageCreateInfo>{};
	auto& raster_state = desc.raster_state;
	auto& depth_state = desc.depth_state;

	auto add_shader = [&shader_mask, &shader_stages](RHIShader* shader, RHIShaderType stage) {
		const auto& vk_shader = static_cast<const VulkanShader&>(*shader);
		shader_stages.emplace_back(vk::PipelineShaderStageCreateFlags{}, vk_shader.getStage(), vk_shader.getHandle(), vk_shader.getDesc().entry_point.c_str());
		shader_mask = shader_mask | stage;
	};

	add_shader(desc.vertex_shader.get(), RHIShaderType::Vertex);
	add_shader(desc.pixel_shader.get(), RHIShaderType::Pixel);

	// vertex input state
	vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
	if (auto* vk_input_layout = static_cast<const VulkanInputLayout*>(desc.input_layout.get()))
		vertex_input_info.setVertexBindingDescriptions(vk_input_layout->getBindings())
		    .setVertexAttributeDescriptions(vk_input_layout->getAttributes());

	// input assembly state
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
	input_assembly_info.setTopology(toVkPrimitiveTopology(desc.primitive_type));

	// viewport state
	vk::PipelineViewportStateCreateInfo viewport_info{};
	viewport_info.setViewportCount(1).setScissorCount(1);

	// rasterization state
	vk::PipelineRasterizationStateCreateInfo rasterizer_info{};
	rasterizer_info.setDepthClampEnable(raster_state.depth_clamp_enable)
	    .setRasterizerDiscardEnable(raster_state.rasterizer_discard_enable)
	    .setPolygonMode(toVkPolygonMode(raster_state.polygon_mode))
	    .setCullMode(toVkCullMode(raster_state.cull_mode))
	    .setFrontFace(toVkFrontFace(raster_state.front_face))
	    .setDepthBiasEnable(raster_state.depth_bias != 0 || raster_state.depth_bias_clamp != 0.0f || raster_state.depth_bias_slope_factor != 0.0f)
	    .setDepthBiasConstantFactor(static_cast<float>(raster_state.depth_bias))
	    .setDepthBiasClamp(raster_state.depth_bias_clamp)
	    .setDepthBiasSlopeFactor(raster_state.depth_bias_slope_factor)
	    .setLineWidth(1.0f);

	// multisample state
	vk::PipelineMultisampleStateCreateInfo multisample_info{};
	multisample_info.setRasterizationSamples(toVkSampleCountFlagBits(framebuffer.getDesc().sample_count))
	    .setAlphaToCoverageEnable(desc.blend_state.alpha_to_coverage_enable);

	// depth stencil state
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_info{};
	depth_stencil_info.setDepthTestEnable(depth_state.depth_test_enable)
	    .setDepthWriteEnable(depth_state.depth_write_enable)
	    .setDepthCompareOp(toVkCompareOp(depth_state.depth_compare_op))
	    .setDepthBoundsTestEnable(depth_state.depth_bounds_test_enable)
	    .setStencilTestEnable(depth_state.stencil_test_enable)
	    .setFront(vk::StencilOpState()
	            .setFailOp(toVkStencilOp(depth_state.front_face_stencil.fail_op))
	            .setPassOp(toVkStencilOp(depth_state.front_face_stencil.pass_op))
	            .setDepthFailOp(toVkStencilOp(depth_state.front_face_stencil.depth_fail_op))
	            .setCompareOp(toVkCompareOp(depth_state.front_face_stencil.stencil_func)))
	    .setBack(vk::StencilOpState()
	            .setFailOp(toVkStencilOp(depth_state.back_face_stencil.fail_op))
	            .setPassOp(toVkStencilOp(depth_state.back_face_stencil.pass_op))
	            .setDepthFailOp(toVkStencilOp(depth_state.back_face_stencil.depth_fail_op))
	            .setCompareOp(toVkCompareOp(depth_state.back_face_stencil.stencil_func)));

	// color blend state
	std::vector<vk::PipelineColorBlendAttachmentState> color_blend_states;
	color_blend_states.reserve(framebuffer.getDesc().color_attachments.size());
	for (size_t i = 0; i < framebuffer.getDesc().color_attachments.size(); ++i) {
		const auto& blend = i < desc.blend_state.blend_descs.size() ? desc.blend_state.blend_descs[i] : RHIColorBlendState::BlendDesc{};
		color_blend_states.emplace_back()
		    .setBlendEnable(blend.blend_enable)
		    .setSrcColorBlendFactor(toVkBlendFactor(blend.src_blend))
		    .setDstColorBlendFactor(toVkBlendFactor(blend.dst_blend))
		    .setColorBlendOp(toVkBlendOp(blend.color_blend_op))
		    .setSrcAlphaBlendFactor(toVkBlendFactor(blend.src_blend_alpha))
		    .setDstAlphaBlendFactor(toVkBlendFactor(blend.dst_blend_alpha))
		    .setAlphaBlendOp(toVkBlendOp(blend.alpha_blend_op))
		    .setColorWriteMask(toVkColorComponentFlags(blend.color_write_mask));
	}
	vk::PipelineColorBlendStateCreateInfo color_blend_info{};
	color_blend_info.setAttachments(color_blend_states);

	// dynamic state
	std::array<vk::DynamicState, 2>    dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamic_info{};
	dynamic_info.setDynamicStates(dynamic_states);

	// pipeline layout
	std::vector<vk::DescriptorSetLayout> set_layouts;
	set_layouts.reserve(desc.binding_layouts.size());
	for (const auto& layout : desc.binding_layouts)
		set_layouts.push_back(static_cast<VulkanBindingLayout*>(layout.get())->getHandle());

	std::vector<vk::PushConstantRange> push_constant_ranges;
	for (const auto& binding_layout : desc.binding_layouts) {
		for (const auto& item : binding_layout->getDesc().bindings) {
			if (item.type != RHIBindingType::PushConstants)
				continue;

			if (!push_constant_ranges.empty())
				throw std::runtime_error("A graphics pipeline currently supports one push constant binding.");

			vk_pipeline->push_constant_visibility = binding_layout->getDesc().visibility;
			vk_pipeline->push_constant_size = item.size;
			push_constant_ranges.emplace_back(toVkShaderStageFlags(binding_layout->getDesc().visibility), 0, item.size);
		}
	}

	vk::PipelineLayoutCreateInfo layout_info{};
	layout_info.setSetLayouts(set_layouts).setPushConstantRanges(push_constant_ranges);

	auto vk_layout = device.createPipelineLayout(layout_info);
	vk_pipeline->layout = vk_layout;
	vk_pipeline->shader_mask = shader_mask;

	//  rendering info
	std::vector<vk::Format> color_formats;
	for (const auto& attachment : framebuffer.getDesc().color_attachments)
		color_formats.push_back(toVkFormat(attachment.format == RHIFormat::Unknown && attachment.texture_view ? attachment.texture_view->getDesc().format : attachment.format));
	auto depth_format = framebuffer.getDesc().depth_attachment.texture_view ? framebuffer.getDesc().depth_attachment.texture_view->getDesc().format : framebuffer.getDesc().depth_format;

	vk::PipelineRenderingCreateInfo rendering_info{};
	rendering_info.setColorAttachmentFormats(color_formats)
	    .setDepthAttachmentFormat(toVkFormat(depth_format))
	    .setStencilAttachmentFormat(getVkImageAspectFlags(depth_format) & vk::ImageAspectFlagBits::eStencil ? toVkFormat(depth_format) : vk::Format::eUndefined);

	// pipeline info
	vk::GraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.setPNext(&rendering_info)
	    .setStages(shader_stages)
	    .setPVertexInputState(&vertex_input_info)
	    .setPInputAssemblyState(&input_assembly_info)
	    .setPViewportState(&viewport_info)
	    .setPRasterizationState(&rasterizer_info)
	    .setPMultisampleState(&multisample_info)
	    .setPDepthStencilState(&depth_stencil_info)
	    .setPColorBlendState(&color_blend_info)
	    .setPDynamicState(&dynamic_info)
	    .setLayout(vk_layout);

	vk_pipeline->pipeline = device.createGraphicsPipeline({}, pipeline_info).value;

	return vk_pipeline;
}

void* VulkanDevice::mapBuffer(RHIBuffer* buffer, RHIAccessMode mode) const
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	if (!vk_buffer)
		return nullptr;

	void* mapped_data{};
	if (vmaMapMemory(allocator, vk_buffer->allocation, &mapped_data) != VK_SUCCESS)
		throw std::runtime_error("Failed to map Vulkan memory.");

	return mapped_data;
}

void VulkanDevice::unmapBuffer(RHIBuffer* buffer) const noexcept
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	if (!vk_buffer)
		return;

	vmaUnmapMemory(allocator, vk_buffer->allocation);
}

void* VulkanDevice::mapStagingTexture(RHIStagingTexture* staging_texture, RHIAccessMode mode) const
{
	auto* staging = static_cast<VulkanStagingTexture*>(staging_texture);
	if (!staging)
		return nullptr;

	void* mapped_data{};
	if (vmaMapMemory(allocator, staging->allocation, &mapped_data) != VK_SUCCESS)
		throw std::runtime_error("Failed to map Vulkan memory.");

	return mapped_data;
}

void VulkanDevice::unmapStagingTexture(RHIStagingTexture* staging_texture) const noexcept
{
	auto* staging = static_cast<VulkanStagingTexture*>(staging_texture);
	if (!staging)
		return;

	vmaUnmapMemory(allocator, staging->allocation);
}

void VulkanDevice::bindBufferMemory(RHIBuffer* buffer, uint64 offset) const noexcept
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	if (!vk_buffer)
		return;

	vmaBindBufferMemory2(allocator, vk_buffer->allocation, offset, vk_buffer->buffer, nullptr);
}

void VulkanDevice::bindTextureMemory(RHITexture* texture, uint64 offset) const noexcept
{
	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	if (!vk_texture)
		return;

	vmaBindImageMemory2(allocator, vk_texture->allocation, offset, vk_texture->image, nullptr);
}

void VulkanDevice::writeBindingSet(RHIBindingSet* set, const RHIBindingSetDesc& desc)
{
	auto* vk_set = static_cast<VulkanBindingSet*>(set);
	if (!vk_set)
		return;

	vk_set->desc = desc;

	std::vector<vk::DescriptorBufferInfo> buffer_infos;
	std::vector<vk::DescriptorImageInfo>  image_infos;
	std::vector<vk::BufferView>           texel_buffer_views;
	std::vector<vk::WriteDescriptorSet>   writes;

	buffer_infos.reserve(desc.bindings.size());
	image_infos.reserve(desc.bindings.size());
	texel_buffer_views.reserve(desc.bindings.size());
	writes.reserve(desc.bindings.size());

	const auto* layout = vk_set->getLayout();
	assert(desc.bindings.size() == layout->getDesc().bindings.size() && "A binding set must provide exactly one item for every layout item.");

	for (const auto& layout_item : layout->getDesc().bindings) {
		const auto binding = std::find_if(desc.bindings.begin(),
		    desc.bindings.end(),
		    [&](const RHIBindingSetItem& item) {
			    return item.slot == layout_item.slot && item.type == layout_item.type;
		    });

		assert(binding != desc.bindings.end() && "A binding set item is missing from its layout.");
	}

	for (const auto& binding : desc.bindings) {
		const auto layout_item = std::find_if(layout->getDesc().bindings.begin(), layout->getDesc().bindings.end(), [&](const RHIBindingLayoutItem& item) { return item.slot == binding.slot && item.type == binding.type; });
		assert(layout_item != layout->getDesc().bindings.end() && "Binding item does not match its layout slot and type.");
		if (binding.type == RHIBindingType::PushConstants) {
			assert(!binding.resource && "Push constant bindings do not reference resources.");
			continue;
		}
		assert(binding.resource && "Resource bindings cannot contain null resources.");

		switch (binding.type) {
		case RHIBindingType::ConstantBuffer:
		case RHIBindingType::StructuredBufferSRV:
		case RHIBindingType::StructuredBufferUAV:
		case RHIBindingType::RawBufferSRV:
		case RHIBindingType::RawBufferUAV:
		{
			auto* buffer_view = dynamic_cast<VulkanBufferView*>(binding.resource.get());
			assert(buffer_view && "Buffer bindings require buffer views.");

			auto& view_desc = buffer_view->getDesc();
			auto* buffer = static_cast<VulkanBuffer*>(&buffer_view->getBuffer());
			assert((binding.type != RHIBindingType::ConstantBuffer || view_desc.type == RHIBufferViewType::Constant) &&
			    "ConstantBuffer bindings require constant buffer views.");
			assert(((binding.type != RHIBindingType::StructuredBufferSRV && binding.type != RHIBindingType::StructuredBufferUAV) ||
			           view_desc.type == RHIBufferViewType::Structured) &&
			    "StructuredBuffer bindings require structured buffer views.");
			assert(((binding.type != RHIBindingType::RawBufferSRV && binding.type != RHIBindingType::RawBufferUAV) ||
			           view_desc.type == RHIBufferViewType::Raw) &&
			    "RawBuffer bindings require raw buffer views.");

			buffer_infos.emplace_back()
			    .setBuffer(buffer->getHandle())
			    .setOffset(view_desc.offset)
			    .setRange(view_desc.size);

			writes.emplace_back()
			    .setDstSet(vk_set->getHandle())
			    .setDstBinding(binding.slot)
			    .setDescriptorCount(1)
			    .setDescriptorType(toVkDescriptorType(binding.type))
			    .setBufferInfo(buffer_infos.back());

			break;
		}

		case RHIBindingType::TypedBufferSRV:
		case RHIBindingType::TypedBufferUAV:
		{
			auto* buffer_view = dynamic_cast<VulkanBufferView*>(binding.resource.get());
			assert(buffer_view && buffer_view->getDesc().type == RHIBufferViewType::Typed && buffer_view->getHandle() &&
			    "TypedBuffer bindings require typed buffer views.");

			texel_buffer_views.push_back(buffer_view->getHandle());
			writes.emplace_back()
			    .setDstSet(vk_set->getHandle())
			    .setDstBinding(binding.slot)
			    .setDescriptorCount(1)
			    .setDescriptorType(toVkDescriptorType(binding.type))
			    .setTexelBufferView(texel_buffer_views.back());
			break;
		}

		case RHIBindingType::TextureSRV:
		case RHIBindingType::TextureUAV:
		{
			auto* texture_view = dynamic_cast<VulkanTextureView*>(binding.resource.get());
			assert(texture_view && "Texture bindings require texture views.");
			assert((binding.type != RHIBindingType::TextureSRV || texture_view->getDesc().type == RHITextureViewType::ShaderResource) &&
			    "TextureSRV bindings require shader resource views.");
			assert((binding.type != RHIBindingType::TextureUAV || texture_view->getDesc().type == RHITextureViewType::UnorderedAccess) &&
			    "TextureUAV bindings require unordered access views.");

			const auto image_layout = binding.type == RHIBindingType::TextureUAV ?
			    vk::ImageLayout::eGeneral :
			    vk::ImageLayout::eShaderReadOnlyOptimal;

			image_infos.emplace_back()
			    .setImageView(texture_view->getHandle())
			    .setImageLayout(image_layout);

			writes.emplace_back()
			    .setDstSet(vk_set->getHandle())
			    .setDstBinding(binding.slot)
			    .setDescriptorCount(1)
			    .setDescriptorType(toVkDescriptorType(binding.type))
			    .setImageInfo(image_infos.back());

			break;
		}

		case RHIBindingType::Sampler:
		{
			auto* sampler = dynamic_cast<VulkanSampler*>(binding.resource.get());
			assert(sampler && "Sampler bindings require sampler resources.");

			image_infos.emplace_back()
			    .setSampler(sampler->getHandle());

			writes.emplace_back()
			    .setDstSet(vk_set->getHandle())
			    .setDstBinding(binding.slot)
			    .setDescriptorCount(1)
			    .setDescriptorType(toVkDescriptorType(binding.type))
			    .setImageInfo(image_infos.back());
			break;
		}

		default:
			assert(false && "Unsupported binding type.");
			break;
		}
	}

	if (!writes.empty())
		device.updateDescriptorSets(writes, {});
}

void VulkanDevice::executeCommandList(RHICommandList* command_list)
{
	auto* vk_command_list = static_cast<VulkanCommandList*>(command_list);

	queue->submit(vk_command_list);
}

void VulkanDevice::destroyBuffer(RHIBuffer* buffer) noexcept
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	if (!vk_buffer)
		return;

	vmaDestroyBuffer(allocator, vk_buffer->buffer, vk_buffer->allocation);

	vk_buffer->buffer = nullptr;
	vk_buffer->allocation = {};
	vk_buffer->allocation_info = {};
}

void VulkanDevice::destroyBufferView(RHIBufferView* view) noexcept
{
	auto* vk_view = static_cast<VulkanBufferView*>(view);
	if (!vk_view)
		return;

	if (vk_view->view)
		device.destroyBufferView(vk_view->view);
	vk_view->view = nullptr;
}

void VulkanDevice::destroyTexture(RHITexture* texture) noexcept
{
	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	if (!vk_texture)
		return;

	vmaDestroyImage(allocator, vk_texture->image, vk_texture->allocation);
	vk_texture->image = nullptr;
	vk_texture->layout = {};
	vk_texture->allocation = {};
	vk_texture->allocation_info = {};
}

void VulkanDevice::destroyTextureView(RHITextureView* view) noexcept
{
	auto* vk_view = static_cast<VulkanTextureView*>(view);
	if (!vk_view)
		return;

	if (vk_view->view)
		device.destroyImageView(vk_view->view);
	vk_view->view = nullptr;
}

void VulkanDevice::destroyStagingTexture(RHIStagingTexture* staging_texture) noexcept
{
	auto* vk_staging = static_cast<VulkanStagingTexture*>(staging_texture);
	if (!vk_staging)
		return;

	vmaDestroyBuffer(allocator, vk_staging->buffer, vk_staging->allocation);

	vk_staging->buffer = nullptr;
	vk_staging->allocation = {};
	vk_staging->allocation_info = {};
}

void VulkanDevice::destroySampler(RHISampler* sampler) noexcept
{
	auto* vk_sampler = static_cast<VulkanSampler*>(sampler);
	if (!vk_sampler)
		return;

	device.destroySampler(vk_sampler->sampler);

	vk_sampler->sampler = nullptr;
}

void VulkanDevice::destroyShader(RHIShader* shader) noexcept
{
	auto* vk_shader = static_cast<VulkanShader*>(shader);
	if (!vk_shader)
		return;

	device.destroyShaderModule(vk_shader->shader);

	vk_shader->shader = nullptr;
}

void VulkanDevice::destroyBindingLayout(RHIBindingLayout* layout) noexcept
{
	auto* vk_layout = static_cast<VulkanBindingLayout*>(layout);
	if (!vk_layout)
		return;

	if (vk_layout->getHandle())
		device.destroyDescriptorSetLayout(vk_layout->getHandle());

	vk_layout->layout = nullptr;
}

void VulkanDevice::destroyBindingSet(RHIBindingSet* set) noexcept
{
	auto* vk_set = static_cast<VulkanBindingSet*>(set);
	if (!vk_set)
		return;

	if (vk_set->getHandle())
		device.destroyDescriptorPool(vk_set->pool);

	vk_set->set = nullptr;
}

void VulkanDevice::destroyGraphicsPipeline(RHIGraphicsPipeline* pipeline) noexcept
{
	auto* vk_pipeline = static_cast<VulkanGraphicsPipeline*>(pipeline);
	if (!vk_pipeline)
		return;

	if (vk_pipeline->getHandle())
		device.destroyPipeline(vk_pipeline->getHandle());

	if (vk_pipeline->getLayout())
		device.destroyPipelineLayout(vk_pipeline->getLayout());
}

}        // namespace Vortex

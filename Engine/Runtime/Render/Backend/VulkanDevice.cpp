#define VMA_IMPLEMENTATION

#include "VulkanDevice.hpp"

#include "VulkanTypes.hpp"
#include "VulkanResources.hpp"
#include "VulkanDescriptor.hpp"
#include "VulkanContext.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanQueue.hpp"

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

VulkanDevice::~VulkanDevice()
{
	device.waitIdle();
	queue.reset();

	vmaDestroyAllocator(allocator);

	device.destroy();
	device = vk::Device{};
}

std::unique_ptr<RHIBuffer> VulkanDevice::createBuffer(const RHIBufferDesc& desc)
{
	assert(desc.size > 0 && "Cannot create an empty Vulkan buffer.");

	auto buffer = std::make_unique<VulkanBuffer>(*this, desc);

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

	VkBuffer vk_buffer{};
	VkResult result = vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &vk_buffer, &buffer->allocation, &buffer->allocation_info);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate Vulkan buffer.");

	buffer->buffer = vk_buffer;

	return buffer;
}

std::unique_ptr<RHITexture> VulkanDevice::createTexture(const RHITextureDesc& desc)
{
	assert(desc.width > 0 && desc.height > 0 && desc.depth > 0 && "Cannot create an empty Vulkan texture.");

	auto texture = std::make_unique<VulkanTexture>(*this, desc);

	VkImageCreateInfo image_info{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
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

	vk::ImageSubresourceRange range{};
	range.setAspectMask(getVkImageAspectFlags(desc.format))
	    .setBaseMipLevel(0)
	    .setLevelCount(desc.mip_levels)
	    .setBaseArrayLayer(0)
	    .setLayerCount(desc.array_layers);

	vk::ImageViewCreateInfo view_info{};
	view_info.setImage(vk_image)
	    .setViewType(toVkImageViewType(desc.dimension))
	    .setFormat(toVkFormat(desc.format))
	    .setSubresourceRange(range);
	vk::ImageView vk_view = device.createImageView(view_info);

	texture->image = vk_image;
	texture->view = vk_view;
	texture->layout = vk::ImageLayout::eUndefined;

	return texture;
}

std::unique_ptr<RHIStagingTexture> VulkanDevice::createStagingTexture(const RHITextureDesc& desc)
{
	assert(desc.width > 0 && desc.height > 0 && desc.depth > 0 && "Cannot create an empty staging texture.");

	auto staging = std::make_unique<VulkanStagingTexture>(*this, desc);

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

std::unique_ptr<RHISampler> VulkanDevice::createSampler(const RHISamplerDesc& desc)
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

	auto sampler = std::make_unique<VulkanSampler>(*this, desc);
	sampler->sampler = device.createSampler(sampler_info);

	return sampler;
}

std::unique_ptr<RHIShader> VulkanDevice::createShader(const RHIShaderDesc& desc)
{
	vk::ShaderModuleCreateInfo shader_info{};
	shader_info.setCodeSize(desc.codes.size())
	    .setPCode(reinterpret_cast<const uint32_t*>(desc.codes.data()));

	auto shader = std::make_unique<VulkanShader>(*this, desc);
	shader->shader = device.createShaderModule(shader_info);
	shader->stage_flags = toVkShaderStageFlagBits(desc.type);

	return shader;
}

std::unique_ptr<RHIFrameBuffer> VulkanDevice::createFrameBuffer(const RHIFrameBufferDesc& desc)
{
	auto framebuffer = std::make_unique<VulkanFrameBuffer>(*this, desc);

	std::vector<RHITexture*>                 attachments;
	std::vector<vk::RenderingAttachmentInfo> color_attachments;
	color_attachments.reserve(desc.color_attachments.size());

	for (const auto& attachment : desc.color_attachments) {
		auto* texture = static_cast<VulkanTexture*>(attachment.texture);
		assert(texture && "A framebuffer color attachment cannot be null.");

		framebuffer->color_attachments_info.emplace_back()
		    .setImageView(texture->getView())
		    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
		    .setLoadOp(vk::AttachmentLoadOp::eLoad)
		    .setStoreOp(vk::AttachmentStoreOp::eStore);

		framebuffer->attachments.push_back(attachment.texture);
	}

	if (desc.depth_attachment.texture) {
		auto* texture = static_cast<VulkanTexture*>(desc.depth_attachment.texture);
		assert(texture && "A framebuffer depth attachment cannot be null.");

		auto layout = desc.depth_attachment.read_only ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eDepthStencilAttachmentOptimal;
		framebuffer->depth_attachment_info.setImageView(texture->getView())
		    .setImageLayout(layout)
		    .setLoadOp(vk::AttachmentLoadOp::eLoad)
		    .setStoreOp(vk::AttachmentStoreOp::eStore);

		if (getVkImageAspectFlags(texture->getDesc().format) & vk::ImageAspectFlagBits::eStencil)
			framebuffer->stencil_attachment_info.setImageView(texture->getView())
			    .setImageLayout(layout)
			    .setLoadOp(vk::AttachmentLoadOp::eLoad)
			    .setStoreOp(vk::AttachmentStoreOp::eStore);

		framebuffer->attachments.push_back(desc.depth_attachment.texture);
	}

	return framebuffer;
}

std::unique_ptr<RHIInputLayout> VulkanDevice::createInputLayout(const RHIInputLayoutDesc& desc)
{
	auto layout = std::make_unique<VulkanInputLayout>(*this, desc);
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

std::unique_ptr<RHIDescriptorLayout> VulkanDevice::createDescriptorLayout(const RHIDescriptorLayoutDesc& desc)
{
	auto layout = std::make_unique<VulkanDescriptorLayout>(*this, desc);

	auto shader_stages = toVkShaderStageFlagBits(desc.visibility);
	for (const auto& item : desc.bindings) {
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
std::unique_ptr<RHIDescriptorSet> VulkanDevice::createDescriptorSet(const RHIDescriptorSetDesc& desc, const RHIDescriptorLayout& layout)
{
	auto set = std::make_unique<VulkanDescriptorSet>(*this, desc, layout);

	auto* vk_layout = static_cast<const VulkanDescriptorLayout*>(&layout);
	auto  vk_desc_layout = vk_layout->getHandle();

	vk::DescriptorPoolCreateInfo pool_info{};
	pool_info.setMaxSets(1)
	    .setPoolSizes(vk_layout->getPoolSizes());
	set->pool = device.createDescriptorPool(pool_info);

	vk::DescriptorSetAllocateInfo alloc_info{};
	alloc_info.setDescriptorPool(set->pool)
	    .setSetLayouts(vk_desc_layout);
	set->set = device.allocateDescriptorSets(alloc_info).front();

	writeDescriptorSet(set.get(), desc);

	return set;
}

std::unique_ptr<RHICommandList> VulkanDevice::createCommandList(const RHICommandListDesc& desc)
{
	if (desc.queue_type != RHICommandQueue::Graphics)
		throw std::runtime_error("Only the graphics Vulkan queue is currently initialized.");

	auto layout = std::make_unique<VulkanCommandList>(*this, desc);

	return layout;
}

std::unique_ptr<RHIGraphicsPipeline> VulkanDevice::createGraphicsPipeline(const RHIGraphicsPipelineDesc& desc, const RHIFrameBuffer& framebuffer)
{
	auto vk_pipeline = std::make_unique<VulkanGraphicsPipeline>(*this, desc);

	auto  shader_mask = RHIShaderType::None;
	auto  shader_stages = std::vector<vk::PipelineShaderStageCreateInfo>{};
	auto& raster_state = desc.raster_state;
	auto& depth_state = desc.depth_state;

	auto add_shader = [&shader_mask, &shader_stages](RHIShader* shader, RHIShaderType stage) {
		const auto& vk_shader = static_cast<const VulkanShader&>(*shader);
		shader_stages.emplace_back(vk::PipelineShaderStageCreateFlags{}, vk_shader.getStage(), vk_shader.getHandle(), vk_shader.getDesc().entry_point.c_str());
		shader_mask = shader_mask | stage;
	};

	add_shader(desc.vertex_shader, RHIShaderType::Vertex);
	add_shader(desc.fragment_shader, RHIShaderType::Fragment);

	// vertex input state
	vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
	if (auto* vk_input_layout = static_cast<const VulkanInputLayout*>(desc.input_layout))
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
	for (auto* layout : desc.binding_layouts)
		set_layouts.push_back(static_cast<VulkanDescriptorLayout*>(layout)->getHandle());

	vk::PipelineLayoutCreateInfo layout_info{};
	layout_info.setSetLayouts(set_layouts);

	auto vk_layout = device.createPipelineLayout(layout_info);
	vk_pipeline->layout = vk_layout;
	vk_pipeline->shader_mask = shader_mask;

	//  rendering info
	std::vector<vk::Format> color_formats;
	for (const auto& attachment : framebuffer.getDesc().color_attachments)
		color_formats.push_back(toVkFormat(attachment.format == RHIFormat::Unknown && attachment.texture ? attachment.texture->getDesc().format : attachment.format));
	auto depth_format = framebuffer.getDesc().depth_attachment.texture ? framebuffer.getDesc().depth_attachment.texture->getDesc().format : framebuffer.getDesc().depth_format;

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

void VulkanDevice::unmapBuffer(RHIBuffer* buffer) const
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

void VulkanDevice::unmapStagingTexture(RHIStagingTexture* staging_texture) const
{
	auto* staging = static_cast<VulkanStagingTexture*>(staging_texture);
	if (!staging)
		return;

	vmaUnmapMemory(allocator, staging->allocation);
}

void VulkanDevice::bindBufferMemory(RHIBuffer* buffer, uint64_t offset) const
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	if (!vk_buffer)
		return;

	vmaBindBufferMemory2(allocator, vk_buffer->allocation, offset, vk_buffer->buffer, nullptr);
}

void VulkanDevice::bindTextureMemory(RHITexture* texture, uint64_t offset) const
{
	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	if (!vk_texture)
		return;

	vmaBindImageMemory2(allocator, vk_texture->allocation, offset, vk_texture->image, nullptr);
}

void VulkanDevice::writeDescriptorSet(RHIDescriptorSet* set, const RHIDescriptorSetDesc& desc)
{
	auto* vk_set = static_cast<VulkanDescriptorSet*>(set);
	if (!vk_set)
		return;

	vk_set->desc = desc;
	vk_set->resources.clear();
	vk_set->resources.reserve(desc.bindings.size());

	std::vector<vk::DescriptorBufferInfo> buffer_infos;
	std::vector<vk::DescriptorImageInfo>  image_infos;
	std::vector<vk::WriteDescriptorSet>   writes;

	buffer_infos.reserve(desc.bindings.size());
	image_infos.reserve(desc.bindings.size());
	writes.reserve(desc.bindings.size());

	for (const auto& binding : desc.bindings) {
		vk_set->resources.push_back(binding.resource);

		switch (binding.type) {
		case RHIDescriptorType::UniformBuffer:
		case RHIDescriptorType::StorageBuffer:
		{
			auto* buffer = static_cast<VulkanBuffer*>(binding.resource);
			if (!buffer)
				break;

			buffer_infos.emplace_back()
			    .setBuffer(buffer->getHandle())
			    .setOffset(0)
			    .setRange(buffer->getDesc().size);

			writes.emplace_back()
			    .setDstSet(vk_set->getHandle())
			    .setDstBinding(binding.slot)
			    .setDescriptorCount(1)
			    .setDescriptorType(toVkDescriptorType(binding.type))
			    .setBufferInfo(buffer_infos.back());

			break;
		}

		case RHIDescriptorType::TextureSRV:
		case RHIDescriptorType::TextureUAV:
		{
			auto* texture = static_cast<VulkanTexture*>(binding.resource);
			if (!texture)
				break;

			const auto image_layout = binding.type == RHIDescriptorType::TextureUAV ?
			    vk::ImageLayout::eGeneral :
			    vk::ImageLayout::eShaderReadOnlyOptimal;

			image_infos.emplace_back()
			    .setImageView(texture->getView())
			    .setImageLayout(image_layout);

			writes.emplace_back()
			    .setDstSet(vk_set->getHandle())
			    .setDstBinding(binding.slot)
			    .setDescriptorCount(1)
			    .setDescriptorType(toVkDescriptorType(binding.type))
			    .setImageInfo(image_infos.back());

			break;
		}

		case RHIDescriptorType::Sampler:
		{
			auto* sampler = static_cast<VulkanSampler*>(binding.resource);
			if (!sampler)
				break;

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
			throw std::runtime_error("Unsupported descriptor binding type.");
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

void VulkanDevice::destroyBuffer(RHIBuffer* buffer)
{
	auto* vk_buffer = static_cast<VulkanBuffer*>(buffer);
	if (!vk_buffer)
		return;

	vmaDestroyBuffer(allocator, vk_buffer->buffer, vk_buffer->allocation);

	vk_buffer->buffer = nullptr;
	vk_buffer->allocation = {};
	vk_buffer->allocation_info = {};
}

void VulkanDevice::destroyTexture(RHITexture* texture)
{
	auto* vk_texture = static_cast<VulkanTexture*>(texture);
	if (!vk_texture)
		return;

	device.destroyImageView(vk_texture->view);
	vmaDestroyImage(allocator, vk_texture->image, vk_texture->allocation);

	vk_texture->view = nullptr;
	vk_texture->image = nullptr;
	vk_texture->layout = {};
	vk_texture->allocation = {};
	vk_texture->allocation_info = {};
}

void VulkanDevice::destroyStagingTexture(RHIStagingTexture* staging_texture)
{
	auto* vk_staging = static_cast<VulkanStagingTexture*>(staging_texture);
	if (!vk_staging)
		return;

	vmaDestroyBuffer(allocator, vk_staging->buffer, vk_staging->allocation);

	vk_staging->buffer = nullptr;
	vk_staging->allocation = {};
	vk_staging->allocation_info = {};
}

void VulkanDevice::destroySampler(RHISampler* sampler)
{
	auto* vk_sampler = static_cast<VulkanSampler*>(sampler);
	if (!vk_sampler)
		return;

	device.destroySampler(vk_sampler->sampler);

	vk_sampler->sampler = nullptr;
}

void VulkanDevice::destroyShader(RHIShader* shader)
{
	auto* vk_shader = static_cast<VulkanShader*>(shader);
	if (!vk_shader)
		return;

	device.destroyShaderModule(vk_shader->shader);

	vk_shader->shader = nullptr;
}

void VulkanDevice::destroyDescriptorLayout(RHIDescriptorLayout* layout)
{
	auto* vk_layout = static_cast<VulkanDescriptorLayout*>(layout);
	if (!vk_layout)
		return;

	if (vk_layout->getHandle())
		device.destroyDescriptorSetLayout(vk_layout->getHandle());

	vk_layout->layout = nullptr;
}

void VulkanDevice::destroyDescriptorSet(RHIDescriptorSet* set)
{
	auto* vk_set = static_cast<VulkanDescriptorSet*>(set);
	if (!vk_set)
		return;

	if (vk_set->getHandle())
		device.destroyDescriptorPool(vk_set->pool);

	vk_set->set = nullptr;
}

void VulkanDevice::destroyGraphicsPipeline(RHIGraphicsPipeline* pipeline)
{
	auto* vk_pipeline = static_cast<VulkanGraphicsPipeline*>(pipeline);
	if (!vk_pipeline)
		return;

	if (vk_pipeline->getHandle())
		device.destroyPipeline(vk_pipeline->getHandle());

	if (vk_pipeline->getLayout())
		device.destroyPipelineLayout(vk_pipeline->getLayout());
}

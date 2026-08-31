module Runtime.Graphics;

import vulkan;

namespace Vortex {

RHIRef<RHIBindingLayout> VulkanDevice::createBindingLayout(const RHIBindingLayoutDesc& desc)
{
	validateRHIBindingLayoutDesc(desc);
	auto       layout = makeRHIRef<VulkanBindingLayout>(*this, desc);
	const auto shader_stages = toVkShaderStageFlags(desc.visibility);

	for (const auto& item : desc.bindings) {
		if (item.type == RHIBindingType::PushConstants)
			continue;

		vk::DescriptorSetLayoutBinding layout_binding{};
		layout_binding.setBinding(item.slot)
		    .setDescriptorCount(1)
		    .setDescriptorType(toVkDescriptorType(item.type))
		    .setStageFlags(shader_stages);

		layout->bindings.push_back(layout_binding);
		if (auto it = std::find_if(layout->pool_sizes.begin(), layout->pool_sizes.end(), [&](const vk::DescriptorPoolSize& pool_size) {
			    return pool_size.type == layout_binding.descriptorType;
		    });
		    it != layout->pool_sizes.end())
			it->descriptorCount += 1;
		else
			layout->pool_sizes.push_back({layout_binding.descriptorType, 1});
	}

	vk::DescriptorSetLayoutCreateInfo layout_info{};
	layout_info.setBindings(layout->bindings);
	layout->layout = device.createDescriptorSetLayout(layout_info);
	return layout;
}

static void writeDescriptors(vk::Device device, const VulkanBindingSet& binding_set)
{
	const auto& desc = binding_set.getDesc();

	std::vector<vk::DescriptorBufferInfo> buffer_infos;
	std::vector<vk::DescriptorImageInfo>  image_infos;
	std::vector<vk::BufferView>           texel_buffer_views;
	std::vector<vk::WriteDescriptorSet>   writes;

	buffer_infos.reserve(desc.bindings.size());
	image_infos.reserve(desc.bindings.size());
	texel_buffer_views.reserve(desc.bindings.size());
	writes.reserve(desc.bindings.size());

	for (const auto& binding : desc.bindings) {
		switch (binding.type) {
		case RHIBindingType::ConstantBuffer:
		case RHIBindingType::StructuredBufferSRV:
		case RHIBindingType::StructuredBufferUAV:
		case RHIBindingType::RawBufferSRV:
		case RHIBindingType::RawBufferUAV:
		{
			auto* buffer_view = dynamic_cast<VulkanBufferView*>(binding.resource.get());
			CHECK(buffer_view, "A Vulkan buffer binding requires a Vulkan buffer view");

			const auto& view_desc = buffer_view->getDesc();
			auto*       buffer = static_cast<VulkanBuffer*>(&buffer_view->getBuffer());
			buffer_infos.emplace_back()
			    .setBuffer(buffer->getHandle())
			    .setOffset(view_desc.offset)
			    .setRange(view_desc.size);

			writes.emplace_back()
			    .setDstSet(binding_set.getHandle())
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
			CHECK(buffer_view && buffer_view->getHandle(),
			    "A Vulkan typed buffer binding requires a typed Vulkan buffer view");

			texel_buffer_views.push_back(buffer_view->getHandle());
			writes.emplace_back()
			    .setDstSet(binding_set.getHandle())
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
			CHECK(texture_view, "A Vulkan texture binding requires a Vulkan texture view");

			const auto image_layout = binding.type == RHIBindingType::TextureUAV ?
			    vk::ImageLayout::eGeneral :
			    vk::ImageLayout::eShaderReadOnlyOptimal;
			image_infos.emplace_back()
			    .setImageView(texture_view->getHandle())
			    .setImageLayout(image_layout);

			writes.emplace_back()
			    .setDstSet(binding_set.getHandle())
			    .setDstBinding(binding.slot)
			    .setDescriptorCount(1)
			    .setDescriptorType(toVkDescriptorType(binding.type))
			    .setImageInfo(image_infos.back());
			break;
		}

		case RHIBindingType::Sampler:
		{
			auto* sampler = dynamic_cast<VulkanSampler*>(binding.resource.get());
			CHECK(sampler, "A Vulkan sampler binding requires a Vulkan sampler");

			image_infos.emplace_back().setSampler(sampler->getHandle());
			writes.emplace_back()
			    .setDstSet(binding_set.getHandle())
			    .setDstBinding(binding.slot)
			    .setDescriptorCount(1)
			    .setDescriptorType(vk::DescriptorType::eSampler)
			    .setImageInfo(image_infos.back());
			break;
		}

		default:
			ERROR(Argument, "Unsupported Vulkan binding type");
		}
	}

	if (!writes.empty())
		device.updateDescriptorSets(writes, {});
}

RHIRef<RHIBindingSet> VulkanDevice::createBindingSet(const RHIBindingSetDesc& desc, const RHIBindingLayout& layout)
{
	validateRHIBindingSetDesc(desc, layout.getDesc());
	auto* vk_layout = dynamic_cast<const VulkanBindingLayout*>(&layout);
	CHECK(vk_layout, "A Vulkan binding set requires a Vulkan binding layout");

	auto                         set = makeRHIRef<VulkanBindingSet>(*this, desc, layout);
	vk::DescriptorPoolCreateInfo pool_info{};
	pool_info.setMaxSets(1)
	    .setPoolSizes(vk_layout->getPoolSizes());
	set->pool = device.createDescriptorPool(pool_info);

	const auto                    descriptor_layout = vk_layout->getHandle();
	vk::DescriptorSetAllocateInfo alloc_info{};
	alloc_info.setDescriptorPool(set->pool)
	    .setSetLayouts(descriptor_layout);
	set->set = device.allocateDescriptorSets(alloc_info).front();

	writeDescriptors(device, *set);
	return set;
}

void VulkanDevice::destroyBindingLayout(VulkanBindingLayout* layout) noexcept
{
	if (layout->layout)
		device.destroyDescriptorSetLayout(layout->layout);
	layout->layout = vk::DescriptorSetLayout{};
}

void VulkanDevice::destroyBindingSet(VulkanBindingSet* set) noexcept
{
	if (set->pool)
		device.destroyDescriptorPool(set->pool);
	set->set = vk::DescriptorSet{};
	set->pool = vk::DescriptorPool{};
}

}        // namespace Vortex

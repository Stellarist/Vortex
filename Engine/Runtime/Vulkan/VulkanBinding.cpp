module Runtime.Vulkan;

import vulkan;

namespace Vortex {

RHIRef<RHIBindingLayout> VulkanDevice::createBindingLayout(const RHIBindingLayoutDesc& desc)
{
	validateRHIBindingLayoutDesc(desc);
	return makeRHIRef<VulkanBindingLayout>(*this, desc);
}

RHIRef<RHIBindingLayout> VulkanDevice::createBindlessLayout(const RHIBindlessLayoutDesc& desc)
{
	validateRHIBindlessLayoutDesc(desc);
	return makeRHIRef<VulkanBindingLayout>(*this, desc);
}

VulkanBindingLayout::VulkanBindingLayout(VulkanDevice& device, RHIBindingLayoutDesc desc) :
    desc(std::move(desc)), device(device)
{
	initialize();
}

VulkanBindingLayout::VulkanBindingLayout(VulkanDevice& device, RHIBindlessLayoutDesc desc) :
    desc(desc), device(device)
{
	initialize();
}

void VulkanBindingLayout::initialize()
{
	auto add_binding = [&](RHIBindingType type, uint32 slot, uint32 count, RHIShaderType visibility) {
		vk::DescriptorSetLayoutBinding layout_binding{};
		layout_binding.setBinding(slot)
		    .setDescriptorCount(count)
		    .setDescriptorType(toVkDescriptorType(type))
		    .setStageFlags(toVkShaderStageFlags(visibility));

		bindings.push_back(layout_binding);
		if (auto it = std::find_if(pool_sizes.begin(), pool_sizes.end(), [&](const vk::DescriptorPoolSize& pool_size) {
			    return pool_size.type == layout_binding.descriptorType;
		    });
		    it != pool_sizes.end())
			it->descriptorCount += count;
		else
			pool_sizes.push_back({layout_binding.descriptorType, count});
	};

	if (const auto* desc = getBindlessDesc()) {
		add_binding(desc->type, desc->slot, desc->max_capacity, desc->visibility);
	} else {
		const auto& binding_desc = *getDesc();
		for (const auto& item : binding_desc.bindings)
			if (item.type != RHIBindingType::PushConstants)
				add_binding(item.type, item.slot, item.array_size, binding_desc.visibility);
	}

	std::vector<vk::DescriptorBindingFlags> flags(bindings.size(),
	    vk::DescriptorBindingFlagBits::ePartiallyBound |
	        vk::DescriptorBindingFlagBits::eUpdateAfterBind |
	        vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending);
	vk::DescriptorSetLayoutBindingFlagsCreateInfo flags_info{};
	flags_info.setBindingFlags(flags);

	vk::DescriptorSetLayoutCreateInfo layout_info{};
	layout_info.setBindings(bindings);
	if (getBindlessDesc())
		layout_info.setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
		    .setPNext(&flags_info);

	CHECK(device.getHandle().getDescriptorSetLayoutSupport(layout_info).supported, "Vulkan device does not support this descriptor set layout");
	layout = device.getHandle().createDescriptorSetLayout(layout_info);
}

static void writeDescriptors(vk::Device device, vk::DescriptorSet set, std::span<const RHIBindingSetItem> bindings)
{
	std::vector<vk::DescriptorBufferInfo> buffer_infos;
	std::vector<vk::DescriptorImageInfo> image_infos;
	std::vector<vk::BufferView> texel_buffer_views;
	std::vector<vk::WriteDescriptorSet> writes;

	buffer_infos.reserve(bindings.size());
	image_infos.reserve(bindings.size());
	texel_buffer_views.reserve(bindings.size());
	writes.reserve(bindings.size());

	for (const auto& binding : bindings) {
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
			auto* buffer = static_cast<VulkanBuffer*>(&buffer_view->getBuffer());
			buffer_infos.emplace_back()
			    .setBuffer(buffer->getHandle())
			    .setOffset(view_desc.offset)
			    .setRange(view_desc.size);

			writes.emplace_back()
			    .setDstSet(set)
			    .setDstBinding(binding.slot)
			    .setDstArrayElement(binding.array_element)
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
			    .setDstSet(set)
			    .setDstBinding(binding.slot)
			    .setDstArrayElement(binding.array_element)
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
			    vk::ImageLayout::eReadOnlyOptimal;
			image_infos.emplace_back()
			    .setImageView(texture_view->getHandle())
			    .setImageLayout(image_layout);

			writes.emplace_back()
			    .setDstSet(set)
			    .setDstBinding(binding.slot)
			    .setDstArrayElement(binding.array_element)
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
			    .setDstSet(set)
			    .setDstBinding(binding.slot)
			    .setDstArrayElement(binding.array_element)
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
	validateRHIBindingSetDesc(desc, layout);
	const auto* vk_layout = dynamic_cast<const VulkanBindingLayout*>(&layout);
	CHECK(vk_layout && &vk_layout->device == this, "A Vulkan binding set requires a layout from the same Vulkan device");

	auto set = makeRHIRef<VulkanBindingSet>(*this, desc, layout);
	vk::DescriptorPoolCreateInfo pool_info{};
	pool_info.setMaxSets(1)
	    .setPoolSizes(vk_layout->getPoolSizes());
	set->pool = device.createDescriptorPool(pool_info);

	const auto descriptor_layout = vk_layout->getHandle();
	vk::DescriptorSetAllocateInfo alloc_info{};
	alloc_info.setDescriptorPool(set->pool)
	    .setSetLayouts(descriptor_layout);
	set->set = device.allocateDescriptorSets(alloc_info).front();

	writeDescriptors(device, set->getHandle(), desc.bindings);
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

RHIRef<RHIDescriptorTable> VulkanDevice::createDescriptorTable(const RHIBindingLayout& layout, uint32 capacity)
{
	auto* native = dynamic_cast<const VulkanBindingLayout*>(&layout);
	CHECK(native && &native->device == this && layout.getBindlessDesc(),
	    "Vulkan descriptor tables require a bindless layout from the same device");
	return makeRHIRef<VulkanDescriptorTable>(*this, layout, capacity);
}

VulkanDescriptorTable::VulkanDescriptorTable(VulkanDevice& device, const RHIBindingLayout& layout, uint32 capacity) :
    layout(&layout), device(device)
{
	CHECK(layout.getBindlessDesc(), "Descriptor tables require a bindless layout");
	resize(capacity);

	const auto& native_layout = static_cast<const VulkanBindingLayout&>(layout);
	vk::DescriptorPoolCreateInfo pool_info{};
	pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
	    .setMaxSets(1)
	    .setPoolSizes(native_layout.getPoolSizes());
	pool = device.getHandle().createDescriptorPool(pool_info);

	const auto descriptor_layout = native_layout.getHandle();
	vk::DescriptorSetAllocateInfo alloc_info{};
	alloc_info.setDescriptorPool(pool).setSetLayouts(descriptor_layout);
	set = device.getHandle().allocateDescriptorSets(alloc_info).front();
}

VulkanDescriptorTable::~VulkanDescriptorTable()
{
	device.getHandle().destroyDescriptorPool(pool);
}

void VulkanDescriptorTable::write(uint32 index, RHIResource* resource)
{
	CHECK(index < capacity, "Descriptor table index {} exceeds capacity {}", index, capacity);
	const auto& binding = *layout->getBindlessDesc();
	RHIBindingSetItem item{.resource = resource, .type = binding.type, .slot = binding.slot, .array_element = index};
	validateRHIBindingSetItem(item, binding);

	writeDescriptors(device.getHandle(), set, std::span{&item, size_t(1)});
}

void VulkanDescriptorTable::clear(uint32 index)
{
	CHECK(index < capacity, "Descriptor table index {} exceeds capacity {}", index, capacity);
}

void VulkanDescriptorTable::resize(uint32 new_capacity, bool keep_contents)
{
	CHECK(new_capacity && new_capacity <= layout->getBindlessDesc()->max_capacity,
	    "Descriptor table capacity must be between 1 and its layout maximum");
	static_cast<void>(keep_contents);
	capacity = new_capacity;
}

}        // namespace Vortex

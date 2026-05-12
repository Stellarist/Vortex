#include "VulkanDescriptor.hpp"

#include "VulkanDevice.hpp"

VulkanDescriptorSet::VulkanDescriptorSet(vk::DescriptorSet set) :
    set(set)
{}

void VulkanDescriptorSet::update(const VulkanDevice& device, uint32_t binding, vk::DescriptorType type, const VulkanBuffer* buffer) const
{
	if (!set || !buffer)
		throw std::runtime_error("Invalid descriptor set or buffer");

	vk::WriteDescriptorSet   write{};
	vk::DescriptorBufferInfo buffer_info{};

	write.setDstSet(set)
	    .setDstBinding(binding)
	    .setDstArrayElement(0)
	    .setDescriptorType(type)
	    .setDescriptorCount(1);

	if (buffer) {
		buffer_info.setBuffer(buffer->get())
		    .setOffset(0)
		    .setRange(buffer->getSize());
		write.setBufferInfo(buffer_info);
	}

	device.logical().updateDescriptorSets(write, {});
}

void VulkanDescriptorSet::update(const VulkanDevice& device, uint32_t binding, vk::DescriptorType type, const VulkanImage* image) const
{
	if (!set || !image)
		throw std::runtime_error("Invalid descriptor set or image");

	vk::WriteDescriptorSet  write{};
	vk::DescriptorImageInfo image_info{};

	image_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
	    .setImageView(image->getView())
	    .setSampler(image->getSampler().get());

	write.setDstSet(set)
	    .setDstBinding(binding)
	    .setDstArrayElement(0)
	    .setDescriptorType(type)
	    .setDescriptorCount(1)
	    .setImageInfo(image_info);

	device.logical().updateDescriptorSets(write, {});
}

vk::DescriptorSet VulkanDescriptorSet::get() const&
{
	return set;
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
    VulkanContext&                                        context,
    std::span<const vk::DescriptorSetLayoutBinding> bindings,
    vk::DescriptorSetLayoutCreateFlags              flags) :
    context(&context)
{
	vk::DescriptorSetLayoutCreateInfo create_info{};
	create_info.setBindings(bindings)
	    .setFlags(flags);

	layout = context.getDevice().logical().createDescriptorSetLayout(create_info);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
	if (context && layout)
		context->getDevice().logical().destroyDescriptorSetLayout(layout);
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& other) noexcept :
    context(std::exchange(other.context, nullptr)),
    layout(std::exchange(other.layout, nullptr))
{}

VulkanDescriptorSetLayout& VulkanDescriptorSetLayout::operator=(VulkanDescriptorSetLayout&& other) noexcept
{
	if (this != &other) {
		if (context && layout)
			context->getDevice().logical().destroyDescriptorSetLayout(layout);

		context = std::exchange(other.context, nullptr);
		layout = std::exchange(other.layout, nullptr);
	}

	return *this;
}

vk::DescriptorSetLayout VulkanDescriptorSetLayout::get() const&
{
	return layout;
}

DescriptorPool::DescriptorPool(
    VulkanContext&                                context,
    uint32_t                                max_sets,
    std::span<const vk::DescriptorPoolSize> pool_sizes,
    vk::DescriptorPoolCreateFlags           flags) :
    context(&context),
    max_sets(max_sets)
{
	vk::DescriptorPoolCreateInfo create_info{};
	create_info.setPoolSizes(pool_sizes)
	    .setMaxSets(max_sets)
	    .setFlags(flags);

	pool = context.getDevice().logical().createDescriptorPool(create_info);
}

DescriptorPool::~DescriptorPool()
{
	if (context && pool)
		context->getDevice().logical().destroyDescriptorPool(pool);
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept :
    pool(std::exchange(other.pool, nullptr)),
    sets(std::move(other.sets)),
    max_sets(other.max_sets),
    context(std::exchange(other.context, nullptr))
{}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
{
	if (this != &other) {
		if (context && pool)
			context->getDevice().logical().destroyDescriptorPool(pool);

		pool = std::exchange(other.pool, nullptr);
		sets = std::move(other.sets);
		max_sets = other.max_sets;
		context = std::exchange(other.context, nullptr);
	}

	return *this;
}

VulkanDescriptorSet DescriptorPool::allocate(const VulkanDescriptorSetLayout& layout)
{
	return allocate({&layout, 1}).front();
}

std::vector<VulkanDescriptorSet> DescriptorPool::allocate(std::span<const VulkanDescriptorSetLayout> layouts)
{
	if (!pool || layouts.empty())
		throw std::runtime_error("Invalid descriptor pool or layouts");

	if (sets.size() + layouts.size() > max_sets)
		throw std::runtime_error("Descriptor pool exceeded maximum number of sets");

	std::vector<vk::DescriptorSetLayout> descriptor_layouts;
	descriptor_layouts.reserve(layouts.size());
	for (const auto& layout : layouts)
		descriptor_layouts.push_back(layout.get());

	vk::DescriptorSetAllocateInfo alloc_info{};
	alloc_info.setDescriptorPool(pool)
	    .setDescriptorSetCount(static_cast<uint32_t>(layouts.size()))
	    .setSetLayouts(descriptor_layouts);
	auto vk_sets = context->getDevice().logical().allocateDescriptorSets(alloc_info);

	std::vector<VulkanDescriptorSet> new_sets;
	new_sets.reserve(vk_sets.size());
	for (const auto& vk_set : vk_sets) {
		sets.emplace_back(vk_set);
		new_sets.emplace_back(vk_set);
	}

	return new_sets;
}

void DescriptorPool::free(VulkanDescriptorSet set)
{
	free(std::span<const VulkanDescriptorSet>{&set, 1});
}

void DescriptorPool::free(std::span<const VulkanDescriptorSet> to_free)
{
	if (!pool || to_free.empty())
		return;

	std::vector<vk::DescriptorSet> vk_sets;
	vk_sets.reserve(to_free.size());
	for (const auto& ds : to_free)
		vk_sets.push_back(ds.get());
	context->getDevice().logical().freeDescriptorSets(pool, vk_sets);

	std::erase_if(sets, [&](VulkanDescriptorSet s) {
		return std::find(to_free.begin(), to_free.end(), s) != to_free.end();
	});
}

size_t DescriptorPool::setsCount() const
{
	return sets.size();
}

void DescriptorPool::reset()
{
	if (pool) {
		context->getDevice().logical().resetDescriptorPool(pool);
		sets.clear();
	}
}

vk::DescriptorPool DescriptorPool::get() const&
{
	return pool;
}

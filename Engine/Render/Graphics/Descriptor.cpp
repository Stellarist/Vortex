#include "Descriptor.hpp"

DescriptorSet::DescriptorSet(Context& context, vk::DescriptorSet set) :
    context(&context),
    set(set)
{}

void DescriptorSet::update(uint32_t binding, vk::DescriptorType type, const Buffer* buffer) const
{
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

	context->getLogicalDevice().updateDescriptorSets(write, {});
}

void DescriptorSet::update(uint32_t binding, vk::DescriptorType type, const Image* texture) const
{
	vk::WriteDescriptorSet  write{};
	vk::DescriptorImageInfo image_info{};

	write.setDstSet(set)
	    .setDstBinding(binding)
	    .setDstArrayElement(0)
	    .setDescriptorType(type)
	    .setDescriptorCount(1);

	if (texture) {
		image_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
		    .setImageView(texture->getView())
		    .setSampler(texture->getSampler().get());
		write.setImageInfo(image_info);
	}

	context->getLogicalDevice().updateDescriptorSets(write, {});
}

vk::DescriptorSet DescriptorSet::get() const&
{
	return set;
}

DescriptorSetLayout::DescriptorSetLayout(
    Context&                                        context,
    std::span<const vk::DescriptorSetLayoutBinding> bindings,
    vk::DescriptorSetLayoutCreateFlags              flags) :
    context(&context)
{
	vk::DescriptorSetLayoutCreateInfo create_info{};
	create_info.setBindings(bindings)
	    .setFlags(flags);

	layout = context.getLogicalDevice().createDescriptorSetLayout(create_info);
}

DescriptorSetLayout::~DescriptorSetLayout()
{
	if (context && layout)
		context->getLogicalDevice().destroyDescriptorSetLayout(layout);
}

vk::DescriptorSetLayout DescriptorSetLayout::get() const&
{
	return layout;
}

DescriptorPool::DescriptorPool(
    Context&                                context,
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

	pool = context.getLogicalDevice().createDescriptorPool(create_info);
}

DescriptorPool::~DescriptorPool()
{
	if (context && pool)
		context->getLogicalDevice().destroyDescriptorPool(pool);
}

size_t DescriptorPool::allocate(const DescriptorSetLayout& layout)
{
	vk::DescriptorSetLayout       descriptor_layout = layout.get();
	vk::DescriptorSetAllocateInfo alloc_info{};
	alloc_info.setDescriptorPool(pool)
	    .setDescriptorSetCount(1)
	    .setSetLayouts(descriptor_layout);

	auto index = sets.size();
	auto new_set = context->getLogicalDevice().allocateDescriptorSets(alloc_info).front();
	sets.push_back(DescriptorSet(*context, new_set));

	return index;
}

std::pair<size_t, size_t> DescriptorPool::allocate(const std::vector<DescriptorSetLayout>& layouts)
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
	auto new_sets = context->getLogicalDevice().allocateDescriptorSets(alloc_info);

	auto start_index = sets.size();
	sets.reserve(sets.size() + new_sets.size());
	for (const auto& set : new_sets)
		sets.push_back(DescriptorSet(*context, set));
	auto end_index = sets.size();

	return {start_index, end_index};
}

void DescriptorPool::reset()
{
	if (pool) {
		context->getLogicalDevice().resetDescriptorPool(pool);
		sets.clear();
	}
}

vk::DescriptorPool DescriptorPool::get() const&
{
	return pool;
}

const DescriptorSet& DescriptorPool::getSet(size_t index) const
{
	if (index >= sets.size())
		throw std::out_of_range("DescriptorSet index out of range");
	return sets[index];
}

std::span<const DescriptorSet> DescriptorPool::getSets(size_t start, size_t end) const
{
	if (start >= sets.size() || end > sets.size() || start > end)
		throw std::out_of_range("DescriptorSet range out of range");

	return {sets.data() + start, end - start};
}

const std::vector<DescriptorSet>& DescriptorPool::getSets() const
{
	return sets;
}

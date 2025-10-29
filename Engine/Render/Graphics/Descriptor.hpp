#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"
#include "Buffer.hpp"
#include "Image.hpp"

class DescriptorSet {
private:
	vk::DescriptorSet set{};

	Context* context{};

public:
	DescriptorSet(
	    Context&          context,
	    vk::DescriptorSet set);

	DescriptorSet(const DescriptorSet&) = delete;
	DescriptorSet& operator=(const DescriptorSet&) = delete;

	DescriptorSet(DescriptorSet&&) noexcept = default;
	DescriptorSet& operator=(DescriptorSet&&) noexcept = default;

	~DescriptorSet() = default;

	void update(uint32_t binding, vk::DescriptorType type, const Buffer* buffer = {}) const;
	void update(uint32_t binding, vk::DescriptorType type, const Image* texture = {}) const;

	vk::DescriptorSet get() const&;
	vk::DescriptorSet get() const&& = delete;
};

class DescriptorSetLayout {
private:
	vk::DescriptorSetLayout layout{};

	Context* context{};

public:
	DescriptorSetLayout(
	    Context&                                        context,
	    std::span<const vk::DescriptorSetLayoutBinding> bindings,
	    vk::DescriptorSetLayoutCreateFlags              flags = {});

	DescriptorSetLayout(const DescriptorSetLayout&) = delete;
	DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

	DescriptorSetLayout(DescriptorSetLayout&& other) noexcept = default;
	DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept = default;

	~DescriptorSetLayout();

	vk::DescriptorSetLayout get() const&;
	vk::DescriptorSetLayout get() const&& = delete;
};

class DescriptorPool {
private:
	vk::DescriptorPool         pool{};
	std::vector<DescriptorSet> sets;
	uint32_t                   max_sets{};

	Context* context{};

public:
	DescriptorPool(
	    Context&                                context,
	    uint32_t                                max_sets,
	    std::span<const vk::DescriptorPoolSize> pool_sizes,
	    vk::DescriptorPoolCreateFlags           flags = {});

	DescriptorPool(const DescriptorPool&) = delete;
	DescriptorPool& operator=(const DescriptorPool&) = delete;

	DescriptorPool(DescriptorPool&& other) noexcept = default;
	DescriptorPool& operator=(DescriptorPool&& other) noexcept = default;

	~DescriptorPool();

	auto allocate(const DescriptorSetLayout& layout) -> size_t;
	auto allocate(const std::vector<DescriptorSetLayout>& layouts) -> std::pair<size_t, size_t>;

	void reset();

	vk::DescriptorPool get() const&;
	vk::DescriptorPool get() const&& = delete;

	const DescriptorSet&              getSet(size_t index) const;
	std::span<const DescriptorSet>    getSets(size_t start, size_t end) const;
	const std::vector<DescriptorSet>& getSets() const;
};

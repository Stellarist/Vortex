#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"
#include "Buffer.hpp"
#include "Image.hpp"

class DescriptorSet {
private:
	vk::DescriptorSet set{};

public:
	DescriptorSet() = default;
	DescriptorSet(vk::DescriptorSet set);
	~DescriptorSet() = default;

	void update(const Device& device, uint32_t binding, vk::DescriptorType type, const Buffer* buffer = {}) const;
	void update(const Device& device, uint32_t binding, vk::DescriptorType type, const Image* image = {}) const;

	vk::DescriptorSet get() const&;
	vk::DescriptorSet get() const&& = delete;

	bool operator==(const DescriptorSet& other) const = default;
};

class DescriptorSetLayout {
private:
	vk::DescriptorSetLayout layout{};

	Context* context{};

public:
	DescriptorSetLayout(Context&                        context,
	    std::span<const vk::DescriptorSetLayoutBinding> bindings,
	    vk::DescriptorSetLayoutCreateFlags              flags = {});
	~DescriptorSetLayout();

	DescriptorSetLayout(const DescriptorSetLayout&) = delete;
	DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

	DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
	DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

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
	~DescriptorPool();

	DescriptorPool(const DescriptorPool&) = delete;
	DescriptorPool& operator=(const DescriptorPool&) = delete;

	DescriptorPool(DescriptorPool&& other) noexcept;
	DescriptorPool& operator=(DescriptorPool&& other) noexcept;

	auto allocate(const DescriptorSetLayout& layout) -> DescriptorSet;
	auto allocate(std::span<const DescriptorSetLayout> layouts) -> std::vector<DescriptorSet>;

	void free(DescriptorSet set);
	void free(std::span<const DescriptorSet> sets);

	void reset();

	size_t setsCount() const;

	vk::DescriptorPool get() const&;
	vk::DescriptorPool get() const&& = delete;
};

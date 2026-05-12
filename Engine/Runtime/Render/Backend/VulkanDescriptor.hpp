#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanImage.hpp"

class VulkanDescriptorSet {
private:
	vk::DescriptorSet set{};

public:
	VulkanDescriptorSet() = default;
	VulkanDescriptorSet(vk::DescriptorSet set);
	~VulkanDescriptorSet() = default;

	void update(const VulkanDevice& device, uint32_t binding, vk::DescriptorType type, const VulkanBuffer* buffer = {}) const;
	void update(const VulkanDevice& device, uint32_t binding, vk::DescriptorType type, const VulkanImage* image = {}) const;

	vk::DescriptorSet get() const&;
	vk::DescriptorSet get() const&& = delete;

	bool operator==(const VulkanDescriptorSet& other) const = default;
};

class VulkanDescriptorSetLayout {
private:
	vk::DescriptorSetLayout layout{};

	VulkanContext* context{};

public:
	VulkanDescriptorSetLayout(VulkanContext&                        context,
	    std::span<const vk::DescriptorSetLayoutBinding> bindings,
	    vk::DescriptorSetLayoutCreateFlags              flags = {});
	~VulkanDescriptorSetLayout();

	VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
	VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

	VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& other) noexcept;
	VulkanDescriptorSetLayout& operator=(VulkanDescriptorSetLayout&& other) noexcept;

	vk::DescriptorSetLayout get() const&;
	vk::DescriptorSetLayout get() const&& = delete;
};

class DescriptorPool {
private:
	vk::DescriptorPool         pool{};
	std::vector<VulkanDescriptorSet> sets;
	uint32_t                   max_sets{};

	VulkanContext* context{};

public:
	DescriptorPool(
	    VulkanContext&                                context,
	    uint32_t                                max_sets,
	    std::span<const vk::DescriptorPoolSize> pool_sizes,
	    vk::DescriptorPoolCreateFlags           flags = {});
	~DescriptorPool();

	DescriptorPool(const DescriptorPool&) = delete;
	DescriptorPool& operator=(const DescriptorPool&) = delete;

	DescriptorPool(DescriptorPool&& other) noexcept;
	DescriptorPool& operator=(DescriptorPool&& other) noexcept;

	auto allocate(const VulkanDescriptorSetLayout& layout) -> VulkanDescriptorSet;
	auto allocate(std::span<const VulkanDescriptorSetLayout> layouts) -> std::vector<VulkanDescriptorSet>;

	void free(VulkanDescriptorSet set);
	void free(std::span<const VulkanDescriptorSet> sets);

	void reset();

	size_t setsCount() const;

	vk::DescriptorPool get() const&;
	vk::DescriptorPool get() const&& = delete;
};

#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"

class VulkanSampler {
	vk::Sampler sampler;

	VulkanContext* context{};

public:
	VulkanSampler(VulkanContext& context);
	~VulkanSampler();

	VulkanSampler(const VulkanSampler&) = delete;
	VulkanSampler& operator=(const VulkanSampler&) = delete;

	VulkanSampler(VulkanSampler&&) noexcept = default;
	VulkanSampler& operator=(VulkanSampler&&) noexcept = default;

	void create();

	vk::Sampler get() const;

	static vk::DescriptorSetLayoutBinding binding(uint32_t binding = {});
};

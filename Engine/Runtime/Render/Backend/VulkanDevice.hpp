#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"

struct VulkanQueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	operator bool() const;
};

class VulkanDevice {
private:
	vk::PhysicalDevice physical_device;
	vk::Device         logical_device;
	vk::Queue          graphics_queue;
	vk::Queue          present_queue;

	std::vector<std::string> extensions{};
	std::vector<std::string> layers{};

	VulkanQueueFamilyIndices queue_family_indices;

	VulkanContext* context{};

	void queryQueueFamilyIndices();

	std::vector<const char*> requestExtensions();
	std::vector<const char*> requestLayers();

public:
	VulkanDevice(VulkanContext& context);
	~VulkanDevice();

	VulkanDevice(const VulkanDevice&) = delete;
	VulkanDevice& operator=(const VulkanDevice&) = delete;

	VulkanDevice(VulkanDevice&&) noexcept = default;
	VulkanDevice& operator=(VulkanDevice&&) noexcept = default;

	void pickPhysicalDevice();
	void createLogicalDevice();

	vk::PhysicalDevice physical() const;
	vk::Device         logical() const;
	vk::Queue          graphicsQueue() const;
	vk::Queue          presentQueue() const;

	uint32_t graphicsQueueIndex() const;
	uint32_t presentQueueIndex() const;
};
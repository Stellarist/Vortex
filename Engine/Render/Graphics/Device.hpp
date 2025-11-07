#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	operator bool() const;
};

class Device {
private:
	vk::PhysicalDevice physical_device;
	vk::Device         logical_device;
	vk::Queue          graphics_queue;
	vk::Queue          present_queue;

	std::vector<std::string> extensions{};
	std::vector<std::string> layers{};

	QueueFamilyIndices queue_family_indices;

	Context* context{};

	void queryQueueFamilyIndices();

	std::vector<const char*> requestExtensions();
	std::vector<const char*> requestLayers();

public:
	Device(Context& context);
	~Device();

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;

	Device(Device&&) noexcept = default;
	Device& operator=(Device&&) noexcept = default;

	void pickPhysicalDevice();
	void createLogicalDevice();

	vk::PhysicalDevice physical() const;
	vk::Device         logical() const;
	vk::Queue          graphicsQueue() const;
	vk::Queue          presentQueue() const;

	uint32_t graphicsQueueIndex() const;
	uint32_t presentQueueIndex() const;
};
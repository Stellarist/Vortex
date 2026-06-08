#pragma once

#include "VulkanDevice.hpp"
#include "Runtime/Render/RHI/RHIDescriptor.hpp"

// descriptor layout
class VulkanDescriptorLayout : public RHIDescriptorLayout {
private:
	RHIDescriptorLayoutDesc desc{};

	vk::DescriptorSetLayout layout{};

	std::vector<vk::DescriptorSetLayoutBinding> bindings{};
	std::vector<vk::DescriptorPoolSize>         pool_sizes{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanDescriptorLayout(VulkanDevice& device, RHIDescriptorLayoutDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanDescriptorLayout() override { device.destroyDescriptorLayout(this); }

	const RHIDescriptorLayoutDesc& getDesc() const override { return desc; }

	vk::DescriptorSetLayout getHandle() const { return layout; }

	const std::vector<vk::DescriptorSetLayoutBinding>& getBindings() const { return bindings; }
	const std::vector<vk::DescriptorPoolSize>&         getPoolSizes() const { return pool_sizes; }
};


// binding set
class VulkanDescriptorSet : public RHIDescriptorSet {
private:
	RHIDescriptorSetDesc desc{};

	const RHIDescriptorLayout* layout{};

	vk::DescriptorSet  set{};
	vk::DescriptorPool pool{};

	std::vector<RHIResource*> resources{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanDescriptorSet(VulkanDevice& device, RHIDescriptorSetDesc desc, const RHIDescriptorLayout& layout) : device(device), desc(std::move(desc)), layout(&layout) {}
	~VulkanDescriptorSet() override { device.destroyDescriptorSet(this); }

	void allocSet();
	void freeSet();

	const RHIDescriptorSetDesc& getDesc() const override { return desc; }

	vk::DescriptorSet  getHandle() const { return set; }
	vk::DescriptorPool getPool() const { return pool; }

	const RHIDescriptorLayout* getLayout() const override { return layout; }
};

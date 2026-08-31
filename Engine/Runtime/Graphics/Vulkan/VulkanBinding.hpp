export module Runtime.Graphics:Vulkan.Binding;

import vulkan;
import Core;
import :RHI.Binding;
import :Vulkan.Device;

export namespace Vortex {

class VulkanBindingLayout : public RHIBindingLayout {
private:
	RHIBindingLayoutDesc desc{};

	vk::DescriptorSetLayout layout{};

	std::vector<vk::DescriptorSetLayoutBinding> bindings{};
	std::vector<vk::DescriptorPoolSize>         pool_sizes{};

	VulkanDevice& device;

	friend class VulkanDevice;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(layout, name);
	}

public:
	VulkanBindingLayout(VulkanDevice& device, RHIBindingLayoutDesc desc) :
	    device(device), desc(std::move(desc)) {}
	~VulkanBindingLayout() override { device.destroyBindingLayout(this); }

	const RHIBindingLayoutDesc& getDesc() const noexcept override { return desc; }

	vk::DescriptorSetLayout getHandle() const noexcept { return layout; }

	const std::vector<vk::DescriptorSetLayoutBinding>& getBindings() const noexcept { return bindings; }
	const std::vector<vk::DescriptorPoolSize>& getPoolSizes() const noexcept { return pool_sizes; }
};

class VulkanBindingSet : public RHIBindingSet {
private:
	RHIBindingSetDesc desc{};

	RHIRef<const RHIBindingLayout> layout{};

	vk::DescriptorSet  set{};
	vk::DescriptorPool pool{};

	VulkanDevice& device;

	friend class VulkanDevice;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(set, name);
		device.setName(pool, name);
	}

public:
	VulkanBindingSet(VulkanDevice& device, RHIBindingSetDesc desc, const RHIBindingLayout& layout) :
	    device(device), desc(std::move(desc)), layout(&layout) {}
	~VulkanBindingSet() override { device.destroyBindingSet(this); }

	const RHIBindingSetDesc& getDesc() const noexcept override { return desc; }

	vk::DescriptorSet getHandle() const noexcept { return set; }
	vk::DescriptorPool getPool() const noexcept { return pool; }

	const RHIBindingLayout* getLayout() const noexcept override
	{
		return layout.get();
	}
};

}        // namespace Vortex

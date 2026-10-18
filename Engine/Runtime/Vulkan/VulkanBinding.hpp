export module Runtime.Vulkan:Binding;

import vulkan;
import Core;
import Runtime.RHI;
import :Device;

export namespace Vortex {

class VulkanBindingLayout : public RHIBindingLayout {
private:
	std::variant<RHIBindingLayoutDesc, RHIBindlessLayoutDesc> desc;

	vk::DescriptorSetLayout layout{};

	std::vector<vk::DescriptorSetLayoutBinding> bindings{};
	std::vector<vk::DescriptorPoolSize> pool_sizes{};

	VulkanDevice& device;

	friend class VulkanDevice;

	void initialize();

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(layout, name);
	}

public:
	VulkanBindingLayout(VulkanDevice& device, RHIBindingLayoutDesc desc);
	VulkanBindingLayout(VulkanDevice& device, RHIBindlessLayoutDesc desc);
	~VulkanBindingLayout() override { device.destroyBindingLayout(this); }

	const RHIBindingLayoutDesc* getDesc() const noexcept override { return std::get_if<RHIBindingLayoutDesc>(&desc); }
	const RHIBindlessLayoutDesc* getBindlessDesc() const noexcept override { return std::get_if<RHIBindlessLayoutDesc>(&desc); }

	vk::DescriptorSetLayout getHandle() const noexcept { return layout; }

	const std::vector<vk::DescriptorSetLayoutBinding>& getBindings() const noexcept { return bindings; }
	const std::vector<vk::DescriptorPoolSize>& getPoolSizes() const noexcept { return pool_sizes; }
};


class VulkanBindingSet : public RHIBindingSet {
private:
	RHIBindingSetDesc desc{};

	RHIRef<const RHIBindingLayout> layout{};

	vk::DescriptorSet set{};
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

	const RHIBindingSetDesc* getDesc() const noexcept override { return &desc; }

	vk::DescriptorSet getHandle() const noexcept { return set; }
	vk::DescriptorPool getPool() const noexcept { return pool; }

	const RHIBindingLayout* getLayout() const noexcept override { return layout.get(); }
};


class VulkanDescriptorTable : public RHIDescriptorTable {
private:
	RHIRef<const RHIBindingLayout> layout;

	vk::DescriptorPool pool{};
	vk::DescriptorSet set{};
	uint32 capacity{};

	VulkanDevice& device;

	friend class VulkanCommandList;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(set, name);
		device.setName(pool, name);
	}

public:
	VulkanDescriptorTable(VulkanDevice& device, const RHIBindingLayout& layout, uint32 capacity);
	~VulkanDescriptorTable() override;

	const RHIBindingSetDesc* getDesc() const noexcept override { return nullptr; }
	const RHIBindingLayout* getLayout() const noexcept override { return layout.get(); }

	uint32 getCapacity() const noexcept override { return capacity; }
	vk::DescriptorSet getHandle() const noexcept { return set; }

	void write(uint32 index, RHIResource* resource) override;
	void clear(uint32 index) override;
	void resize(uint32 capacity, bool keep_contents = true) override;
};

}        // namespace Vortex

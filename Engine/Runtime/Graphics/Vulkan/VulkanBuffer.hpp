module;

#include <vk_mem_alloc.h>

export module Runtime.Graphics:Vulkan.Buffer;

import vulkan;
import Core;
import :Vulkan.Device;
import :RHI.Buffer;

export namespace Vortex {

class VulkanBuffer : public RHIBuffer {
private:
	RHIBufferDesc desc{};
	RHIResourceState state{Unknown};

	vk::Buffer buffer{};
	VmaAllocation allocation{};
	VmaAllocationInfo allocation_info{};

	VulkanDevice& device;

	friend class VulkanDevice;
	friend class VulkanCommandList;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(buffer, name);
	}

public:
	VulkanBuffer(VulkanDevice& device, RHIBufferDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanBuffer() override { device.destroyBuffer(this); }

	const RHIBufferDesc& getDesc() const noexcept override { return desc; }
	RHIResourceState getState() const noexcept override { return state; }

	vk::Buffer getHandle() const noexcept { return buffer; }
};


class VulkanBufferView : public RHIBufferView {
private:
	RHIBufferViewDesc desc{};

	vk::BufferView view{};

	VulkanDevice& device;

	friend class VulkanDevice;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(view, name);
	}

public:
	VulkanBufferView(VulkanDevice& device, RHIBufferViewDesc desc) :
	    device(device), desc(std::move(desc)) {}
	~VulkanBufferView() override { device.destroyBufferView(this); }

	const RHIBufferViewDesc& getDesc() const noexcept override { return desc; }
	RHIBuffer& getBuffer() const noexcept override { return *desc.buffer; }

	vk::BufferView getHandle() const noexcept { return view; }
};

}        // namespace Vortex

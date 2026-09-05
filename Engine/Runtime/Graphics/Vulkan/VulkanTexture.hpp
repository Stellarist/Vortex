module;

#include <vk_mem_alloc.h>

export module Runtime.Graphics:Vulkan.Texture;

import vulkan;
import Core;
import :Vulkan.Device;
import :RHI.Texture;

export namespace Vortex {

class VulkanTexture : public RHITexture {
private:
	RHITextureDesc desc{};
	RHIResourceState state{Unknown};

	vk::Image image{};
	vk::ImageLayout layout{};
	VmaAllocation allocation{};
	VmaAllocationInfo allocation_info{};

	VulkanDevice& device;

	friend class VulkanDevice;
	friend class VulkanCommandList;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(image, name);
	}

public:
	VulkanTexture(VulkanDevice& device, RHITextureDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanTexture() override { device.destroyTexture(this); }

	const RHITextureDesc& getDesc() const noexcept override { return desc; }
	RHIResourceState getState() const noexcept override { return state; }

	vk::Image getHandle() const noexcept { return image; }
	vk::ImageLayout getLayout() const noexcept { return layout; }
};


class VulkanTextureView : public RHITextureView {
private:
	RHITextureViewDesc desc{};

	vk::ImageView view{};

	VulkanDevice& device;

	friend class VulkanDevice;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(view, name);
	}

public:
	VulkanTextureView(VulkanDevice& device, RHITextureViewDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanTextureView() override { device.destroyTextureView(this); }

	const RHITextureViewDesc& getDesc() const noexcept override { return desc; }
	RHITexture& getTexture() const noexcept override { return *desc.texture; }

	vk::ImageView getHandle() const noexcept { return view; }
};


class VulkanStagingTexture : public RHIStagingTexture {
private:
	RHITextureDesc desc{};

	vk::Buffer buffer{};
	VmaAllocation allocation{};
	VmaAllocationInfo allocation_info{};

	VulkanDevice& device;

	friend class VulkanDevice;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(buffer, name);
	}

public:
	VulkanStagingTexture(VulkanDevice& device, RHITextureDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanStagingTexture() override { device.destroyStagingTexture(this); }

	const RHITextureDesc& getDesc() const noexcept override { return desc; }

	vk::Buffer getHandle() const noexcept { return buffer; }
};


class VulkanSampler : public RHISampler {
private:
	RHISamplerDesc desc{};

	vk::Sampler sampler{};

	VulkanDevice& device;

	friend class VulkanDevice;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(sampler, name);
	}

public:
	VulkanSampler(VulkanDevice& device, RHISamplerDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanSampler() override { device.destroySampler(this); }

	const RHISamplerDesc& getDesc() const noexcept override { return desc; }

	vk::Sampler getHandle() const noexcept { return sampler; }
};

}        // namespace Vortex

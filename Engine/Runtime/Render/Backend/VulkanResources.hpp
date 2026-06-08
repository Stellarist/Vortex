#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanDevice.hpp"
#include "Runtime/Render/RHI/RHIResources.hpp"

// Allocation
class VulkanAllocation {
	VmaAllocation     allocation{};
	VmaAllocationInfo allocation_info{};

	friend class VulkanDevice;
};


// Buffer
class VulkanBuffer : public RHIBuffer, public VulkanAllocation {
private:
	RHIBufferDesc    desc{};
	RHIResourceState state{Unknown};

	vk::Buffer buffer{};

	VulkanDevice& device;

	friend class VulkanDevice;
	friend class VulkanCommandList;

public:
	VulkanBuffer(VulkanDevice& device, RHIBufferDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanBuffer() override { device.destroyBuffer(this); }

	const RHIBufferDesc& getDesc() const override { return desc; }
	RHIResourceState     getState() const { return state; }

	vk::Buffer getHandle() const { return buffer; }
};


// Texture
class VulkanTexture : public RHITexture, public VulkanAllocation {
private:
	RHITextureDesc   desc{};
	RHIResourceState state{Unknown};

	vk::Image       image{};
	vk::ImageView   view{};
	vk::ImageLayout layout{};

	VulkanDevice& device;

	friend class VulkanDevice;
	friend class VulkanCommandList;

public:
	VulkanTexture(VulkanDevice& device, RHITextureDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanTexture() override { device.destroyTexture(this); }

	const RHITextureDesc& getDesc() const override { return desc; }
	RHIResourceState      getState() const { return state; }

	vk::Image       getHandle() const { return image; }
	vk::ImageView   getView() const { return view; }
	vk::ImageLayout getLayout() const { return layout; }
};


// Staging Texture
class VulkanStagingTexture : public RHIStagingTexture, public VulkanAllocation {
private:
	RHITextureDesc desc{};

	vk::Buffer buffer{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanStagingTexture(VulkanDevice& device, RHITextureDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanStagingTexture() override { device.destroyStagingTexture(this); }

	const RHITextureDesc& getDesc() const override { return desc; }

	vk::Buffer getHandle() const { return buffer; }
};


// Sampler
class VulkanSampler : public RHISampler {
private:
	RHISamplerDesc desc{};

	vk::Sampler sampler{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanSampler(VulkanDevice& device, RHISamplerDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanSampler() override { device.destroySampler(this); }

	const RHISamplerDesc& getDesc() const override { return desc; }

	vk::Sampler getHandle() const { return sampler; }
};


// Shader
class VulkanShader : public RHIShader {
private:
	RHIShaderDesc desc{};

	vk::ShaderModule        shader{};
	vk::ShaderStageFlagBits stage_flags{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanShader(VulkanDevice& device, RHIShaderDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanShader() override { device.destroyShader(this); }

	const RHIShaderDesc& getDesc() const override { return desc; }

	vk::ShaderModule        getHandle() const { return shader; }
	vk::ShaderStageFlagBits getStage() const { return stage_flags; }
};


// FrameBuffer
class VulkanFrameBuffer : public RHIFrameBuffer {
private:
	RHIFrameBufferDesc desc{};

	std::vector<vk::RenderingAttachmentInfo> color_attachments_info{};
	vk::RenderingAttachmentInfo              depth_attachment_info{};
	vk::RenderingAttachmentInfo              stencil_attachment_info{};

	std::vector<RHITexture*> attachments{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanFrameBuffer(VulkanDevice& device, RHIFrameBufferDesc desc) : device(device), desc(std::move(desc)) {};
	~VulkanFrameBuffer() override = default;

	const RHIFrameBufferDesc& getDesc() const override { return desc; };

	const std::vector<RHITexture*> getAttachments() const { return attachments; }

	const std::vector<vk::RenderingAttachmentInfo>& getColorAttachmentsInfo() const { return color_attachments_info; }

	const vk::RenderingAttachmentInfo& getDepthAttachmentInfo() const { return depth_attachment_info; }
	const vk::RenderingAttachmentInfo& getStencilAttachmentInfo() const { return stencil_attachment_info; }
};

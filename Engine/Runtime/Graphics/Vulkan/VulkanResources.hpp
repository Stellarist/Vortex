module;

#include <vk_mem_alloc.h>

export module Runtime.Graphics:Vulkan.Resources;

import vulkan;
import Core;
import :Vulkan.Device;
import :RHI.Buffer;
import :RHI.Framebuffer;
import :RHI.Sampler;
import :RHI.Shader;
import :RHI.Texture;

export namespace Vortex {

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

	const RHIBufferDesc& getDesc() const noexcept override { return desc; }
	RHIResourceState     getState() const noexcept { return state; }

	vk::Buffer getHandle() const noexcept
	{
		return buffer;
	}
};


// Buffer View
class VulkanBufferView : public RHIBufferView {
private:
	RHIBufferViewDesc desc{};

	vk::BufferView view{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanBufferView(VulkanDevice& device, RHIBufferViewDesc desc) :
	    device(device), desc(std::move(desc)) {}
	~VulkanBufferView() override { device.destroyBufferView(this); }

	const RHIBufferViewDesc& getDesc() const noexcept override { return desc; }
	RHIBuffer&               getBuffer() const noexcept override { return *desc.buffer; }

	vk::BufferView getHandle() const noexcept { return view; }
};


// Texture
class VulkanTexture : public RHITexture, public VulkanAllocation {
private:
	RHITextureDesc   desc{};
	RHIResourceState state{Unknown};

	vk::Image       image{};
	vk::ImageLayout layout{};

	VulkanDevice& device;

	friend class VulkanDevice;
	friend class VulkanCommandList;

public:
	VulkanTexture(VulkanDevice& device, RHITextureDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanTexture() override { device.destroyTexture(this); }

	const RHITextureDesc& getDesc() const noexcept override { return desc; }
	RHIResourceState      getState() const noexcept { return state; }

	vk::Image       getHandle() const noexcept { return image; }
	vk::ImageLayout getLayout() const noexcept { return layout; }
};


// Texture View
class VulkanTextureView : public RHITextureView {
private:
	RHITextureViewDesc desc{};

	vk::ImageView view{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanTextureView(VulkanDevice& device, RHITextureViewDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanTextureView() override { device.destroyTextureView(this); }

	const RHITextureViewDesc& getDesc() const noexcept override { return desc; }
	RHITexture&               getTexture() const noexcept override { return *desc.texture; }

	vk::ImageView getHandle() const noexcept { return view; }
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

	const RHITextureDesc& getDesc() const noexcept override { return desc; }

	vk::Buffer getHandle() const noexcept { return buffer; }
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

	const RHISamplerDesc& getDesc() const noexcept override { return desc; }

	vk::Sampler getHandle() const noexcept { return sampler; }
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

	const RHIShaderDesc& getDesc() const noexcept override { return desc; }

	vk::ShaderModule        getHandle() const noexcept { return shader; }
	vk::ShaderStageFlagBits getStage() const noexcept { return stage_flags; }
};


// Framebuffer
class VulkanFramebuffer : public RHIFramebuffer {
private:
	RHIFramebufferDesc desc{};

	std::vector<vk::RenderingAttachmentInfo> color_attachments_info{};
	vk::RenderingAttachmentInfo              depth_attachment_info{};
	vk::RenderingAttachmentInfo              stencil_attachment_info{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanFramebuffer(VulkanDevice& device, RHIFramebufferDesc desc) :
	    device(device), desc(std::move(desc)) {};
	~VulkanFramebuffer() override = default;

	const RHIFramebufferDesc& getDesc() const noexcept override { return desc; };

	const std::vector<vk::RenderingAttachmentInfo>& getColorAttachmentsInfo() const noexcept { return color_attachments_info; }

	const vk::RenderingAttachmentInfo& getDepthAttachmentInfo() const noexcept { return depth_attachment_info; }
	const vk::RenderingAttachmentInfo& getStencilAttachmentInfo() const noexcept { return stencil_attachment_info; }
};

}        // namespace Vortex

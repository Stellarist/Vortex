#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include "Runtime/Render/RHI/RHIDevice.hpp"

class VulkanContext;
class VulkanQueue;
class VulkanCommandList;

class VulkanDevice : public RHIDevice {
private:
	vk::Device device{};

	VmaAllocator allocator{};

	std::unique_ptr<VulkanQueue> queue{};

	VulkanContext* context{};

public:
	VulkanDevice(VulkanContext& context, vk::Device device);
	~VulkanDevice();

	std::unique_ptr<RHIBuffer>         createBuffer(const RHIBufferDesc& desc) override;
	std::unique_ptr<RHITexture>        createTexture(const RHITextureDesc& desc) override;
	std::unique_ptr<RHIStagingTexture> createStagingTexture(const RHITextureDesc& desc) override;
	std::unique_ptr<RHISampler>        createSampler(const RHISamplerDesc& desc) override;
	std::unique_ptr<RHIShader>         createShader(const RHIShaderDesc& desc) override;
	std::unique_ptr<RHIFrameBuffer>    createFrameBuffer(const RHIFrameBufferDesc& desc) override;

	std::unique_ptr<RHIInputLayout>      createInputLayout(const RHIInputLayoutDesc& desc) override;
	std::unique_ptr<RHIDescriptorLayout> createDescriptorLayout(const RHIDescriptorLayoutDesc& desc) override;
	std::unique_ptr<RHIDescriptorSet>    createDescriptorSet(const RHIDescriptorSetDesc& desc, const RHIDescriptorLayout& layout) override;
	std::unique_ptr<RHICommandList>      createCommandList(const RHICommandListDesc& desc) override;
	std::unique_ptr<RHIGraphicsPipeline> createGraphicsPipeline(const RHIGraphicsPipelineDesc& desc, const RHIFrameBuffer& framebuffer) override;

	void* mapBuffer(RHIBuffer* buffer, RHIAccessMode mode) const override;
	void  unmapBuffer(RHIBuffer* buffer) const override;
	void* mapStagingTexture(RHIStagingTexture* staging_texture, RHIAccessMode mode) const override;
	void  unmapStagingTexture(RHIStagingTexture* staging_texture) const override;
	void  bindBufferMemory(RHIBuffer* buffer, uint64_t offset = {}) const override;
	void  bindTextureMemory(RHITexture* texture, uint64_t offset = {}) const override;

	void writeDescriptorSet(RHIDescriptorSet* set, const RHIDescriptorSetDesc& desc) override;
	void executeCommandList(RHICommandList* command_list) override;

	void destroyBuffer(RHIBuffer* buffer);
	void destroyTexture(RHITexture* texture);
	void destroyStagingTexture(RHIStagingTexture* staging_texture);
	void destroySampler(RHISampler* sampler);
	void destroyShader(RHIShader* shader);

	void destroyDescriptorLayout(RHIDescriptorLayout* layout);
	void destroyDescriptorSet(RHIDescriptorSet* set);
	void destroyGraphicsPipeline(RHIGraphicsPipeline* pipeline);

	vk::Device getHandle() const { return device; }

	VulkanContext& getContext() const { return *context; }
	VulkanQueue&   getQueue() const { return *queue; }

	void waitIdle() override { device.waitIdle(); }
};

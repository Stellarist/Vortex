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
	~VulkanDevice() noexcept override;

	RHIRef<RHIBuffer>         createBuffer(const RHIBufferDesc& desc) override;
	RHIRef<RHIBufferView>     createBufferView(const RHIBufferViewDesc& desc) override;
	RHIRef<RHITexture>        createTexture(const RHITextureDesc& desc) override;
	RHIRef<RHITextureView>    createTextureView(const RHITextureViewDesc& desc) override;
	RHIRef<RHIStagingTexture> createStagingTexture(const RHITextureDesc& desc) override;
	RHIRef<RHISampler>        createSampler(const RHISamplerDesc& desc) override;
	RHIRef<RHIShader>         createShader(const RHIShaderDesc& desc, std::span<const std::byte> bytecode) override;
	RHIRef<RHIFramebuffer>    createFramebuffer(const RHIFramebufferDesc& desc) override;

	RHIRef<RHIInputLayout>      createInputLayout(const RHIInputLayoutDesc& desc) override;
	RHIRef<RHIBindingLayout>    createBindingLayout(const RHIBindingLayoutDesc& desc) override;
	RHIRef<RHIBindingSet>       createBindingSet(const RHIBindingSetDesc& desc, const RHIBindingLayout& layout) override;
	RHIRef<RHICommandList>      createCommandList(const RHICommandListDesc& desc) override;
	RHIRef<RHIGraphicsPipeline> createGraphicsPipeline(const RHIGraphicsPipelineDesc& desc, const RHIFramebuffer& framebuffer) override;

	void* mapBuffer(RHIBuffer* buffer, RHIAccessMode mode) const override;
	void  unmapBuffer(RHIBuffer* buffer) const noexcept override;
	void* mapStagingTexture(RHIStagingTexture* staging_texture, RHIAccessMode mode) const override;
	void  unmapStagingTexture(RHIStagingTexture* staging_texture) const noexcept override;
	void  bindBufferMemory(RHIBuffer* buffer, uint64_t offset = {}) const noexcept override;
	void  bindTextureMemory(RHITexture* texture, uint64_t offset = {}) const noexcept override;

	void writeBindingSet(RHIBindingSet* set, const RHIBindingSetDesc& desc) override;
	void executeCommandList(RHICommandList* command_list) override;

	void destroyBuffer(RHIBuffer* buffer) noexcept;
	void destroyBufferView(RHIBufferView* view) noexcept;
	void destroyTexture(RHITexture* texture) noexcept;
	void destroyTextureView(RHITextureView* view) noexcept;
	void destroyStagingTexture(RHIStagingTexture* staging_texture) noexcept;
	void destroySampler(RHISampler* sampler) noexcept;
	void destroyShader(RHIShader* shader) noexcept;

	void destroyBindingLayout(RHIBindingLayout* layout) noexcept;
	void destroyBindingSet(RHIBindingSet* set) noexcept;
	void destroyGraphicsPipeline(RHIGraphicsPipeline* pipeline) noexcept;

	vk::Device getHandle() const noexcept { return device; }

	VulkanContext& getContext() const noexcept { return *context; }
	VulkanQueue&   getQueue() const noexcept { return *queue; }

	void waitIdle() override { device.waitIdle(); }
};

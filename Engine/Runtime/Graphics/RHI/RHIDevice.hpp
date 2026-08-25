export module Runtime.Graphics:RHI.Device;

import Core;
import :RHI.Command;

export namespace Vortex {

class RHIDevice {
public:
	RHIDevice() = default;
	virtual ~RHIDevice() noexcept = default;

	RHIDevice(const RHIDevice&) = delete;
	RHIDevice& operator=(const RHIDevice&) = delete;

	RHIDevice(RHIDevice&&) = delete;
	RHIDevice& operator=(RHIDevice&&) = delete;

	virtual RHIRef<RHIBuffer>         createBuffer(const RHIBufferDesc& desc) = 0;
	virtual RHIRef<RHIBufferView>     createBufferView(const RHIBufferViewDesc& desc) = 0;
	virtual RHIRef<RHITexture>        createTexture(const RHITextureDesc& desc) = 0;
	virtual RHIRef<RHITextureView>    createTextureView(const RHITextureViewDesc& desc) = 0;
	virtual RHIRef<RHISampler>        createSampler(const RHISamplerDesc& desc) = 0;
	virtual RHIRef<RHIShader>         createShader(const RHIShaderDesc& desc, std::span<const std::byte> bytecode) = 0;
	virtual RHIRef<RHIStagingTexture> createStagingTexture(const RHITextureDesc& desc) = 0;
	virtual RHIRef<RHIFramebuffer>    createFramebuffer(const RHIFramebufferDesc& desc) = 0;

	virtual RHIRef<RHIInputLayout>      createInputLayout(const RHIInputLayoutDesc& desc) = 0;
	virtual RHIRef<RHIBindingLayout>    createBindingLayout(const RHIBindingLayoutDesc& desc) = 0;
	virtual RHIRef<RHIBindingSet>       createBindingSet(const RHIBindingSetDesc& desc, const RHIBindingLayout& layout) = 0;
	virtual RHIRef<RHICommandList>      createCommandList(const RHICommandListDesc& desc) = 0;
	virtual RHIRef<RHIGraphicsPipeline> createGraphicsPipeline(const RHIGraphicsPipelineDesc& desc, const RHIFramebuffer& framebuffer) = 0;

	virtual void* mapBuffer(RHIBuffer* buffer, RHIAccessMode mode) const = 0;
	virtual void  unmapBuffer(RHIBuffer* buffer) const noexcept = 0;
	virtual void* mapStagingTexture(RHIStagingTexture* staging_texture, RHIAccessMode mode) const = 0;
	virtual void  unmapStagingTexture(RHIStagingTexture* staging_texture) const noexcept = 0;
	virtual void  bindBufferMemory(RHIBuffer* buffer, uint64 offset = {}) const noexcept = 0;
	virtual void  bindTextureMemory(RHITexture* texture, uint64 offset = {}) const noexcept = 0;

	virtual void writeBindingSet(RHIBindingSet* set, const RHIBindingSetDesc& desc) = 0;
	virtual void executeCommandList(RHICommandList* command_list) = 0;

	virtual void waitIdle() = 0;
};

}        // namespace Vortex

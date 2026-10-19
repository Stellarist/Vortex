export module Runtime.D3D12:Device;

import d3d12;
import Core;
import Runtime.RHI;
import :Types;
import :Binding;

export namespace Vortex {

class D3D12Queue;
class D3D12Context;
class D3D12Buffer;
class D3D12Texture;
class D3D12TextureView;
class D3D12StagingTexture;
class D3D12BindingSet;
class D3D12DescriptorTable;

class D3D12Device : public RHIDevice {
private:
	D3D12Ptr<ID3D12Device> device{};

	D3D12Ptr<D3D12MA::Allocator> allocator{};
	std::unique_ptr<D3D12Queue> queue{};

	std::unique_ptr<D3D12DescriptorAllocator> render_target_descriptors{};
	std::unique_ptr<D3D12DescriptorAllocator> depth_stencil_descriptors{};
	std::unique_ptr<D3D12DescriptorAllocator> resource_descriptors{};
	std::unique_ptr<D3D12DescriptorAllocator> sampler_descriptors{};
	std::unique_ptr<D3D12DescriptorHeap> resource_heap{};
	std::unique_ptr<D3D12DescriptorHeap> sampler_heap{};

	D3D12Context* context{};

public:
	D3D12Device(D3D12Context& context, D3D12Ptr<ID3D12Device> native_device);
	~D3D12Device() noexcept override;

	RHIAPI getAPI() const noexcept override { return RHIAPI::D3D12; }

	RHIRef<RHIBuffer> createBuffer(const RHIBufferDesc& desc) override;
	RHIRef<RHIBufferView> createBufferView(const RHIBufferViewDesc& desc) override;
	RHIRef<RHITexture> createTexture(const RHITextureDesc& desc) override;
	RHIRef<RHITextureView> createTextureView(const RHITextureViewDesc& desc) override;
	RHIRef<RHIStagingTexture> createStagingTexture(const RHITextureDesc& desc, RHIAccessMode access) override;
	RHIRef<RHISampler> createSampler(const RHISamplerDesc& desc) override;
	RHIRef<RHIShader> createShader(const RHIShaderDesc& desc, std::span<const std::byte> bytecode) override;
	RHIRef<RHIFramebuffer> createFramebuffer(const RHIFramebufferDesc& desc) override;

	RHIRef<RHIInputLayout> createInputLayout(const RHIInputLayoutDesc& desc) override;
	RHIRef<RHIBindingLayout> createBindingLayout(const RHIBindingLayoutDesc& desc) override;
	RHIRef<RHIBindingLayout> createBindlessLayout(const RHIBindlessLayoutDesc& desc) override;
	RHIRef<RHIBindingSet> createBindingSet(const RHIBindingSetDesc& desc, const RHIBindingLayout& layout) override;
	RHIRef<RHIDescriptorTable> createDescriptorTable(const RHIBindingLayout& layout, uint32 capacity) override;
	RHIRef<RHICommandList> createCommandList(const RHICommandListDesc& desc) override;
	RHIRef<RHIGraphicsPipeline> createGraphicsPipeline(const RHIGraphicsPipelineDesc& desc) override;
	RHIRef<RHIComputePipeline> createComputePipeline(const RHIComputePipelineDesc& desc) override;

	void destroyBuffer(D3D12Buffer* buffer) noexcept;
	void destroyTexture(D3D12Texture* texture) noexcept;
	void destroyTextureView(D3D12TextureView* view) noexcept;
	void destroyStagingTexture(D3D12StagingTexture* texture) noexcept;
	void destroyBindingSet(D3D12BindingSet* set) noexcept;

	void* mapBuffer(RHIBuffer* buffer, RHIAccessMode mode) const override;
	void unmapBuffer(RHIBuffer* buffer) const noexcept override;
	void* mapStagingTexture(RHIStagingTexture* texture, RHIAccessMode mode,
	    uint64& row_pitch, uint64& depth_pitch, uint32 mip = 0, uint32 layer = 0) const override;
	void unmapStagingTexture(RHIStagingTexture* texture) const noexcept override;

	void executeCommandList(RHICommandList* command) override;

	ID3D12Device* getHandle() const noexcept { return device.Get(); }
	D3D12Context& getContext() const noexcept { return *context; }

	D3D12DescriptorAllocator& getRenderTargetDescriptors() const noexcept { return *render_target_descriptors; }
	D3D12DescriptorAllocator& getDepthStencilDescriptors() const noexcept { return *depth_stencil_descriptors; }
	D3D12DescriptorAllocator& getResourceDescriptors() const noexcept { return *resource_descriptors; }
	D3D12DescriptorAllocator& getSamplerDescriptors() const noexcept { return *sampler_descriptors; }
	D3D12DescriptorHeap& getResourceHeap() const noexcept { return *resource_heap; }
	D3D12DescriptorHeap& getSamplerHeap() const noexcept { return *sampler_heap; }

	D3D12Queue& getQueue() const noexcept { return *queue; }

	void waitIdle() override;
};

}        // namespace Vortex

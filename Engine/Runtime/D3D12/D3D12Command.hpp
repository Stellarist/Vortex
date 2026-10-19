export module Runtime.D3D12:Command;

import d3d12;
import Core;
import Runtime.RHI;
import :Types;
import :Device;
import :Binding;

export namespace Vortex {

class D3D12CommandBuffer {
private:
	D3D12Ptr<ID3D12GraphicsCommandList> command{};
	D3D12Ptr<ID3D12CommandAllocator> allocator{};

	std::vector<RHIRef<RHIResource>> resources{};

	uint64 submission{};
	bool closed{};

	friend class D3D12CommandList;
	friend class D3D12Queue;

	void reset();
	void trackResource(RHIResource* resource);
	void resetResources() noexcept;

public:
	D3D12CommandBuffer(ID3D12Device* device);

	ID3D12GraphicsCommandList* getHandle() const noexcept { return command.Get(); }
};


class D3D12CommandList : public RHICommandList {
private:
	RHICommandListDesc desc{};

	RHIGraphicsState graphics_state{};
	RHIComputeState compute_state{};

	std::shared_ptr<D3D12CommandBuffer> current_command{};
	std::unordered_map<RHIBuffer*, D3D12_RESOURCE_STATES> buffer_states{};

	const D3D12RootSignature* root{};

	bool rendering{};
	bool recording{};

	D3D12Device& device;

	friend class D3D12Queue;

	void useBuffer(RHIBuffer* buffer, RHIResourceState state, std::string_view operation);
	void bindSets(const std::vector<RHIRef<RHIBindingSet>>& sets, const std::vector<RHIRef<RHIBindingLayout>>& layouts);
	void bindFramebuffer(RHIFramebuffer* target);
	void barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

public:
	D3D12CommandList(D3D12Device& device, RHICommandListDesc desc) :
	    desc(std::move(desc)), device(device) {}

	const RHICommandListDesc& getDesc() const noexcept { return desc; }

	std::shared_ptr<D3D12CommandBuffer> getCurrentCommand() const noexcept { return current_command; }

	void open() override;
	void close() override;
	void clear() override;

	void clearTexture(RHITexture* texture, const RHIColor& color) override;
	void clearDepthTexture(RHITexture* texture, bool clear_depth, float depth, bool clear_stencil, uint8 stencil) override;
	void copyTexture(RHITexture* dst, const RHITextureSlice& dst_slice, RHITexture* src, const RHITextureSlice& src_slice) override;
	void copyTexture(RHIStagingTexture* dst, const RHITextureSlice& dst_slice, RHITexture* src, const RHITextureSlice& src_slice) override;
	void copyTexture(RHITexture* dst, const RHITextureSlice& dst_slice, RHIStagingTexture* src, const RHITextureSlice& src_slice) override;
	void writeTexture(RHITexture* texture, const RHITextureSlice& slice, const void* data, uint64 size) override;
	void transitionTexture(RHITexture* texture, RHIResourceState before, RHIResourceState after) override;

	void clearBuffer(RHIBuffer* buffer, uint32 value) override;
	void copyBuffer(RHIBuffer* dst, uint64 dst_offset, RHIBuffer* src, uint64 src_offset, uint64 size) override;
	void writeBuffer(RHIBuffer* buffer, uint64 offset, const void* data, uint64 size) override;
	void transitionBuffer(RHIBuffer* buffer, RHIResourceState before, RHIResourceState after) override;

	void setPushConstants(const void* data, size_t size) override;
	void draw(const RHIDrawArguments& args) override;
	void drawIndexed(const RHIDrawArguments& args) override;
	void dispatch(uint32 x, uint32 y = 1, uint32 z = 1) override;

	void beginRendering(RHIFramebuffer* target) override;
	void endRendering() override;
	void setGraphicsState(const RHIGraphicsState& state) override;
	void setComputeState(const RHIComputeState& state) override;

	void beginDebugLabel(std::string_view name) override;
	void endDebugLabel() override;
};

}        // namespace Vortex

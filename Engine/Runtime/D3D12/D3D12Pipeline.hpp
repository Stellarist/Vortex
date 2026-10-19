export module Runtime.D3D12:Pipeline;

import d3d12;
import Core;
import Runtime.RHI;
import :Types;
import :Binding;

export namespace Vortex {

class D3D12InputLayout : public RHIInputLayout {
private:
	RHIInputLayoutDesc desc{};

	std::vector<std::string> semantics{};
	std::vector<D3D12_INPUT_ELEMENT_DESC> attributes{};

	friend class D3D12Device;

public:
	D3D12InputLayout(RHIInputLayoutDesc desc) : desc(std::move(desc)) {}

	const RHIVertexAttributeDesc& getAttributeDesc(uint32 index) const override { return desc.attribute_descs.at(index); }
	uint32 getAttributeCount() const noexcept override { return static_cast<uint32>(desc.attribute_descs.size()); }

	const std::vector<RHIVertexBindingDesc>& getBindings() const noexcept { return desc.binding_descs; }
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& getAttributes() const noexcept { return attributes; }
};


class D3D12GraphicsPipeline : public RHIGraphicsPipeline {
private:
	RHIGraphicsPipelineDesc desc{};

	std::unique_ptr<D3D12RootSignature> root{};
	D3D12Ptr<ID3D12PipelineState> pipeline{};

	friend class D3D12Device;

protected:
	void applyName(const std::string& name) noexcept override
	{
		setD3D12Name(pipeline.Get(), name);
	}

public:
	D3D12GraphicsPipeline(RHIGraphicsPipelineDesc desc) : desc(std::move(desc)) {}

	const RHIGraphicsPipelineDesc& getDesc() const noexcept override { return desc; }

	const D3D12RootSignature& getRootSignature() const noexcept { return *root; }
	ID3D12PipelineState* getHandle() const noexcept { return pipeline.Get(); }
};


class D3D12ComputePipeline : public RHIComputePipeline {
private:
	RHIComputePipelineDesc desc{};

	std::unique_ptr<D3D12RootSignature> root{};
	D3D12Ptr<ID3D12PipelineState> pipeline{};

	friend class D3D12Device;

protected:
	void applyName(const std::string& name) noexcept override
	{
		setD3D12Name(pipeline.Get(), name);
	}

public:
	D3D12ComputePipeline(RHIComputePipelineDesc desc) : desc(std::move(desc)) {}

	const RHIComputePipelineDesc& getDesc() const noexcept override { return desc; }

	const D3D12RootSignature& getRootSignature() const noexcept { return *root; }
	ID3D12PipelineState* getHandle() const noexcept { return pipeline.Get(); }
};

}        // namespace Vortex

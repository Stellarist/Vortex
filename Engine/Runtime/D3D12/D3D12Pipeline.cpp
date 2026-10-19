module Runtime.D3D12;

import d3d12;

namespace Vortex {

RHIRef<RHIInputLayout> D3D12Device::createInputLayout(const RHIInputLayoutDesc& desc)
{
	validateRHIInputLayoutDesc(desc);
	auto layout = makeRHIRef<D3D12InputLayout>(desc);

	layout->semantics.reserve(desc.attribute_descs.size());
	layout->attributes.reserve(desc.attribute_descs.size());
	for (const auto& attribute : desc.attribute_descs) {
		const auto binding = std::ranges::find(desc.binding_descs, attribute.binding, &RHIVertexBindingDesc::binding);
		CHECK(binding != desc.binding_descs.end(), "Missing vertex buffer binding");

		auto name = attribute.name;
		const auto suffix = name.find_last_not_of("0123456789") + 1;
		const uint32 index = suffix < name.size() ? static_cast<uint32>(std::stoul(name.substr(suffix))) : 0;
		name.resize(suffix);
		layout->semantics.push_back(std::move(name));
		layout->attributes.push_back({layout->semantics.back().c_str(),
		    index,
		    toDXGIFormat(attribute.format),
		    attribute.binding,
		    attribute.offset,
		    binding->instance ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		    binding->instance ? 1u : 0u});
	}

	return layout;
}

RHIRef<RHIGraphicsPipeline> D3D12Device::createGraphicsPipeline(const RHIGraphicsPipelineDesc& desc)
{
	validateRHIGraphicsPipelineDesc(desc);
	CHECK(desc.raster_state.polygon_mode != RHIPolygonMode::Point && !desc.raster_state.rasterizer_discard_enable && !desc.depth_state.depth_bounds_test_enable,
	    "D3D12 RHI does not support point polygon mode, rasterizer discard, or depth bounds testing");

	auto pipeline = makeRHIRef<D3D12GraphicsPipeline>(desc);
	pipeline->root = std::make_unique<D3D12RootSignature>(device.Get(), desc.binding_layouts, true);

	std::span<const D3D12_INPUT_ELEMENT_DESC> attributes;
	if (auto* input_layout = static_cast<const D3D12InputLayout*>(desc.input_layout.get()))
		attributes = input_layout->getAttributes();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC native{
	    .pRootSignature = pipeline->root->handle.Get(),
	    .VS = static_cast<D3D12Shader*>(desc.vertex_shader.get())->getBytecode(),
	    .PS = desc.pixel_shader ? static_cast<D3D12Shader*>(desc.pixel_shader.get())->getBytecode() : D3D12_SHADER_BYTECODE{},
	    .BlendState = toD3D12BlendDesc(desc.blend_state),
	    .SampleMask = ~0u,
	    .RasterizerState = toD3D12RasterizerDesc(desc.raster_state, desc.framebuffer_info.sample_count),
	    .DepthStencilState = toD3D12DepthStencilDesc(desc.depth_state),
	    .InputLayout = {attributes.data(), static_cast<UINT>(attributes.size())},
	    .PrimitiveTopologyType = toD3D12PrimitiveTopologyType(desc.primitive_type),
	    .NumRenderTargets = static_cast<UINT>(desc.framebuffer_info.color_formats.size()),
	    .DSVFormat = toDXGIFormat(desc.framebuffer_info.depth_format),
	    .SampleDesc = {.Count = desc.framebuffer_info.sample_count},
	};

	for (uint32 i = 0; i < native.NumRenderTargets; ++i)
		native.RTVFormats[i] = toDXGIFormat(desc.framebuffer_info.color_formats[i]);

	const HRESULT result = device->CreateGraphicsPipelineState(&native, IID_ID3D12PipelineState, &pipeline->pipeline);
	CHECK(SUCCEEDED(result), "CreateGraphicsPipelineState failed (HRESULT 0x{:08X})", static_cast<uint32>(result));

	return pipeline;
}

RHIRef<RHIComputePipeline> D3D12Device::createComputePipeline(const RHIComputePipelineDesc& desc)
{
	validateRHIComputePipelineDesc(desc);

	auto pipeline = makeRHIRef<D3D12ComputePipeline>(desc);
	pipeline->root = std::make_unique<D3D12RootSignature>(device.Get(), desc.binding_layouts, false);

	const D3D12_COMPUTE_PIPELINE_STATE_DESC native{
	    .pRootSignature = pipeline->root->handle.Get(),
	    .CS = static_cast<D3D12Shader*>(desc.compute_shader.get())->getBytecode(),
	};

	const HRESULT result = device->CreateComputePipelineState(&native, IID_ID3D12PipelineState, &pipeline->pipeline);
	CHECK(SUCCEEDED(result), "CreateComputePipelineState failed (HRESULT 0x{:08X})", static_cast<uint32>(result));

	return pipeline;
}

}        // namespace Vortex

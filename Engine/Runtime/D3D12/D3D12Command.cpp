module Runtime.D3D12;

import d3d12;

namespace Vortex {

D3D12CommandBuffer::D3D12CommandBuffer(ID3D12Device* device)
{
	const HRESULT allocator_result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_ID3D12CommandAllocator, &allocator);
	CHECK(SUCCEEDED(allocator_result), "CreateCommandAllocator failed (HRESULT 0x{:08X})", static_cast<uint32>(allocator_result));

	const HRESULT command_result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_ID3D12GraphicsCommandList, &command);
	CHECK(SUCCEEDED(command_result), "CreateCommandList failed (HRESULT 0x{:08X})", static_cast<uint32>(command_result));
}

void D3D12CommandBuffer::reset()
{
	const HRESULT allocator_result = allocator->Reset();
	CHECK(SUCCEEDED(allocator_result), "Reset command allocator failed (HRESULT 0x{:08X})", static_cast<uint32>(allocator_result));

	const HRESULT command_result = command->Reset(allocator.Get(), nullptr);
	CHECK(SUCCEEDED(command_result), "Reset command list failed (HRESULT 0x{:08X})", static_cast<uint32>(command_result));

	submission = 0;
	closed = false;
}

void D3D12CommandBuffer::trackResource(RHIResource* resource)
{
	if (std::ranges::none_of(resources, [resource](const auto& item) { return item.get() == resource; }))
		resources.emplace_back(resource);
}

void D3D12CommandBuffer::resetResources() noexcept
{
	resources.clear();
}


RHIRef<RHICommandList> D3D12Device::createCommandList(const RHICommandListDesc& desc)
{
	validateRHICommandListDesc(desc);

	return makeRHIRef<D3D12CommandList>(*this, desc);
}

void D3D12Device::executeCommandList(RHICommandList* command)
{
	auto* native = dynamic_cast<D3D12CommandList*>(command);
	CHECK(native, "D3D12 execution requires a D3D12 command list");

	queue->submit(native);
}


void D3D12CommandList::bindSets(const std::vector<RHIRef<RHIBindingSet>>& sets, const std::vector<RHIRef<RHIBindingLayout>>& layouts)
{
	validateRHIBindingSets(sets, layouts);
	auto& resource_heap = device.getResourceHeap();
	auto& sampler_heap = device.getSamplerHeap();
	ID3D12DescriptorHeap* heaps[] = {resource_heap.getHandle(), sampler_heap.getHandle()};
	current_command->getHandle()->SetDescriptorHeaps(2, heaps);

	for (uint32 i = 0; i < sets.size(); ++i) {
		D3D12DescriptorIndex first_resource = D3D12InvalidDescriptorIndex;
		D3D12DescriptorIndex first_sampler = D3D12InvalidDescriptorIndex;
		if (auto* table = dynamic_cast<D3D12DescriptorTable*>(sets[i].get())) {
			CHECK(&table->device == &device, "D3D12 descriptor table belongs to another device");
			current_command->trackResource(table);
			if (table->layout->getBindlessDesc()->type == RHIBindingType::Sampler)
				first_sampler = table->first_descriptor;
			else
				first_resource = table->first_descriptor;
		} else {
			auto* set = dynamic_cast<D3D12BindingSet*>(sets[i].get());
			CHECK(set && &set->device == &device, "D3D12 binding requires a binding set from the same device");
			current_command->trackResource(set);
			first_resource = set->resources.first;
			first_sampler = set->samplers.first;
			for (const auto& item : set->getDesc()->bindings) {
				if (auto* view = dynamic_cast<D3D12BufferView*>(item.resource.get())) {
					auto& buffer = view->getBuffer();
					const auto state = item.type == RHIBindingType::ConstantBuffer               ? ConstantBuffer :
					    toD3D12DescriptorRangeType(item.type) == D3D12_DESCRIPTOR_RANGE_TYPE_UAV ? UnorderedAccess :
					                                                                               ShaderResource;
					useBuffer(&buffer, state, "bind buffer");
				}
			}
		}

		const auto& table = root->tables[i];
		if (table.resources != D3D12InvalidRootParameter) {
			CHECK(first_resource != D3D12InvalidDescriptorIndex, "Missing D3D12 resource descriptor range");
			if (compute_state.pipeline)
				current_command->getHandle()->SetComputeRootDescriptorTable(table.resources, resource_heap.gpu(first_resource));
			else
				current_command->getHandle()->SetGraphicsRootDescriptorTable(table.resources, resource_heap.gpu(first_resource));
		}
		if (table.samplers != D3D12InvalidRootParameter) {
			CHECK(first_sampler != D3D12InvalidDescriptorIndex, "Missing D3D12 sampler descriptor range");
			if (compute_state.pipeline)
				current_command->getHandle()->SetComputeRootDescriptorTable(table.samplers, sampler_heap.gpu(first_sampler));
			else
				current_command->getHandle()->SetGraphicsRootDescriptorTable(table.samplers, sampler_heap.gpu(first_sampler));
		}
	}
}

void D3D12CommandList::bindFramebuffer(RHIFramebuffer* target)
{
	auto* native_framebuffer = dynamic_cast<D3D12Framebuffer*>(target);
	CHECK(native_framebuffer, "D3D12 rendering requires a D3D12 framebuffer");
	current_command->trackResource(target);

	const auto& native = *native_framebuffer;
	for (const auto& attachment : target->getDesc().color_attachments) {
		auto& texture = attachment.texture_view->getTexture();
		validateRHITextureState(texture, RenderTarget, "beginRendering");
	}

	const auto& depth = target->getDesc().depth_attachment;
	if (depth.texture_view) {
		auto& texture = depth.texture_view->getTexture();
		validateRHITextureState(texture, depth.read_only ? DepthRead : DepthWrite, "beginRendering");
	}

	const auto& colors = native.getColorAttachments();
	const auto& dsv = native.getDepthAttachment();
	current_command->getHandle()->OMSetRenderTargets(static_cast<UINT>(colors.size()), colors.data(), false, depth.texture_view ? &dsv : nullptr);
	graphics_state.framebuffer = target;
	rendering = true;
}

void D3D12CommandList::barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER native{};
	if (before != after) {
		native.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		native.Transition = {resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, before, after};
	} else if (after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
		native.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		native.UAV.pResource = resource;
	} else
		return;

	current_command->getHandle()->ResourceBarrier(1, &native);
}

void D3D12CommandList::open()
{
	CHECK(!recording, "D3D12 command list is already open");
	CHECK(!current_command || current_command->submission, "Cannot discard an unsubmitted D3D12 command list");

	current_command.reset();
	current_command = device.getQueue().acquire();
	buffer_states.clear();
	recording = true;
	clear();
}

void D3D12CommandList::close()
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	clear();

	const HRESULT result = current_command->getHandle()->Close();
	CHECK(SUCCEEDED(result), "Close command list failed (HRESULT 0x{:08X})", static_cast<uint32>(result));
	current_command->closed = true;
	recording = false;
}

void D3D12CommandList::clear()
{
	endRendering();
	graphics_state = {};
	compute_state = {};
	root = nullptr;
}

void D3D12CommandList::setPushConstants(const void* data, size_t size)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	CHECK(root && root->constants != D3D12InvalidRootParameter && data && size && size % 4 == 0 && size <= root->constant_size, "Invalid D3D12 push constants");

	if (compute_state.pipeline)
		current_command->getHandle()->SetComputeRoot32BitConstants(root->constants, static_cast<UINT>(size / 4), data, 0);
	else
		current_command->getHandle()->SetGraphicsRoot32BitConstants(root->constants, static_cast<UINT>(size / 4), data, 0);
}

void D3D12CommandList::draw(const RHIDrawArguments& args)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	CHECK(graphics_state.pipeline && rendering, "Draw requires graphics state and a framebuffer");

	current_command->getHandle()->DrawInstanced(args.vertex_count, args.instance_count, args.start_vertex, args.start_instance);
}

void D3D12CommandList::drawIndexed(const RHIDrawArguments& args)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	CHECK(graphics_state.pipeline && rendering && graphics_state.index_buffer.buffer, "Indexed draw requires graphics state, framebuffer and index buffer");

	current_command->getHandle()->DrawIndexedInstanced(args.vertex_count, args.instance_count, args.start_index, static_cast<INT>(args.start_vertex), args.start_instance);
}

void D3D12CommandList::dispatch(uint32 x, uint32 y, uint32 z)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	CHECK(compute_state.pipeline && x && y && z && x <= 65535 && y <= 65535 && z <= 65535, "Invalid compute dispatch");
	endRendering();

	current_command->getHandle()->Dispatch(x, y, z);
}

void D3D12CommandList::beginRendering(RHIFramebuffer* target)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	CHECK(!rendering, "A D3D12 render pass is already active");
	bindFramebuffer(target);

	const auto& native = static_cast<const D3D12Framebuffer&>(*target);
	const auto& colors = target->getDesc().color_attachments;
	for (uint32 index = 0; index < colors.size(); ++index) {
		const auto& attachment = colors[index];
		if (attachment.load_op == RHILoadOp::Clear)
			current_command->getHandle()->ClearRenderTargetView(native.getColorAttachments()[index], &attachment.clear_value.color.r, 0, nullptr);
	}

	const auto& depth = target->getDesc().depth_attachment;
	if (depth.texture_view && depth.load_op == RHILoadOp::Clear) {
		CHECK(!depth.read_only, "A read-only depth attachment cannot be cleared");
		const auto flags = hasRHIStencil(depth.texture_view->getDesc().format) ? D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL : D3D12_CLEAR_FLAG_DEPTH;
		current_command->getHandle()->ClearDepthStencilView(native.getDepthAttachment(), flags, depth.clear_value.depth, static_cast<UINT8>(depth.clear_value.stencil), 0, nullptr);
	}
}

void D3D12CommandList::endRendering()
{
	graphics_state.framebuffer = nullptr;
	rendering = false;
}

void D3D12CommandList::setGraphicsState(const RHIGraphicsState& state)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	auto* pipeline = dynamic_cast<D3D12GraphicsPipeline*>(state.pipeline.get());
	CHECK(pipeline, "D3D12 graphics state requires a D3D12 graphics pipeline");
	CHECK(state.framebuffer, "Missing graphics framebuffer");
	CHECK(!rendering || graphics_state.framebuffer == state.framebuffer, "Cannot change framebuffer inside a render pass");

	const auto& depth_attachment = state.framebuffer->getDesc().depth_attachment;
	if (depth_attachment.texture_view && depth_attachment.read_only) {
		const auto& depth = pipeline->getDesc().depth_state;
		CHECK(!depth.depth_write_enable, "A read-only depth attachment requires depth writes to be disabled");
		const auto keepsStencil = [](const RHIDepthStencilState::StencilOp& face) {
			return face.fail_op == RHIStencilOp::Keep && face.depth_fail_op == RHIStencilOp::Keep && face.pass_op == RHIStencilOp::Keep;
		};
		CHECK(!depth.stencil_test_enable || (keepsStencil(depth.front_face_stencil) && keepsStencil(depth.back_face_stencil)),
		    "A read-only depth/stencil attachment requires stencil writes to be disabled");
	}

	if (!rendering)
		beginRendering(state.framebuffer.get());

	current_command->trackResource(pipeline);
	graphics_state = state;
	compute_state = {};
	root = &pipeline->getRootSignature();
	current_command->getHandle()->SetPipelineState(pipeline->getHandle());
	current_command->getHandle()->SetGraphicsRootSignature(root->handle.Get());
	current_command->getHandle()->OMSetStencilRef(0);
	current_command->getHandle()->IASetPrimitiveTopology(toD3D12PrimitiveTopology(pipeline->getDesc().primitive_type));
	bindSets(state.binding_sets, pipeline->getDesc().binding_layouts);

	const auto viewport_state = normalizeRHIViewportState(state.viewport_state, state.framebuffer->getDesc());
	std::vector<D3D12_VIEWPORT> viewports;
	std::vector<D3D12_RECT> scissors;
	for (const auto& view : viewport_state.viewports)
		viewports.push_back(toD3D12Viewport(view));
	for (const auto& rect : viewport_state.scissors)
		scissors.push_back(toD3D12Rect(rect));

	current_command->getHandle()->RSSetViewports(static_cast<UINT>(viewports.size()), viewports.data());
	current_command->getHandle()->RSSetScissorRects(static_cast<UINT>(scissors.size()), scissors.data());

	for (const auto& binding : state.vertex_buffers) {
		CHECK(binding.buffer && pipeline->getDesc().input_layout, "Vertex buffer requires an input layout");
		const auto* input_layout = dynamic_cast<D3D12InputLayout*>(pipeline->getDesc().input_layout.get());
		CHECK(input_layout, "D3D12 vertex bindings require a D3D12 input layout");
		const auto& bindings = input_layout->getBindings();
		const auto layout = std::ranges::find(bindings, binding.slot, &RHIVertexBindingDesc::binding);
		CHECK(layout != bindings.end() && binding.offset < binding.buffer->getDesc().size, "Invalid vertex buffer binding");
		useBuffer(binding.buffer.get(), VertexBuffer, "bind vertex buffer");
		D3D12_VERTEX_BUFFER_VIEW view{static_cast<D3D12Buffer*>(binding.buffer.get())->getHandle()->GetGPUVirtualAddress() + binding.offset,
		    static_cast<UINT>(binding.buffer->getDesc().size - binding.offset), layout->stride};
		current_command->getHandle()->IASetVertexBuffers(binding.slot, 1, &view);
	}

	if (state.index_buffer.buffer) {
		const auto& binding = state.index_buffer;
		CHECK(binding.offset < binding.buffer->getDesc().size && (binding.format == RHIFormat::R16_UINT || binding.format == RHIFormat::R32_UINT), "Invalid index buffer binding");
		useBuffer(binding.buffer.get(), IndexBuffer, "bind index buffer");
		D3D12_INDEX_BUFFER_VIEW view{static_cast<D3D12Buffer*>(binding.buffer.get())->getHandle()->GetGPUVirtualAddress() + binding.offset,
		    static_cast<UINT>(binding.buffer->getDesc().size - binding.offset), toDXGIFormat(binding.format)};
		current_command->getHandle()->IASetIndexBuffer(&view);
	}
}

void D3D12CommandList::setComputeState(const RHIComputeState& state)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	endRendering();

	auto* pipeline = dynamic_cast<D3D12ComputePipeline*>(state.pipeline.get());
	CHECK(pipeline, "D3D12 compute state requires a D3D12 compute pipeline");

	current_command->trackResource(pipeline);
	compute_state = state;
	graphics_state = {};
	root = &pipeline->getRootSignature();
	current_command->getHandle()->SetPipelineState(pipeline->getHandle());
	current_command->getHandle()->SetComputeRootSignature(root->handle.Get());
	bindSets(state.binding_sets, pipeline->getDesc().binding_layouts);
}

void D3D12CommandList::beginDebugLabel(std::string_view name)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	const std::string label_name(name);
	PIXBeginEvent(current_command->getHandle(), 0, label_name.c_str());
}

void D3D12CommandList::endDebugLabel()
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	PIXEndEvent(current_command->getHandle());
}

}        // namespace Vortex

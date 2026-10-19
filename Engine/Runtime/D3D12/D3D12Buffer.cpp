module Runtime.D3D12;

import d3d12;

namespace Vortex {

RHIRef<RHIBuffer> D3D12Device::createBuffer(const RHIBufferDesc& desc)
{
	validateRHIBufferDesc(desc);

	auto buffer = makeRHIRef<D3D12Buffer>(*this, desc);

	uint64 allocation_size = desc.size;
	if ((desc.usage & RHIBufferUsage::ConstantBuffer) != RHIBufferUsage::None) {
		constexpr uint64 alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		CHECK(desc.size <= std::numeric_limits<uint64>::max() - (alignment - 1), "D3D12 constant buffer size overflows alignment");
		allocation_size = (desc.size + alignment - 1) & ~(alignment - 1);
	}

	auto buffer_info = toD3D12BufferDesc(allocation_size);
	if (desc.access == RHIAccessMode::None && (desc.usage & RHIBufferUsage::StorageBuffer) != RHIBufferUsage::None)
		buffer_info.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	const D3D12MA::ALLOCATION_DESC allocation_desc{.HeapType = toD3D12HeapType(desc.access)};

	const auto initial_state = getD3D12InitialBufferState(desc.access);
	const HRESULT result = allocator->CreateResource(&allocation_desc, &buffer_info, initial_state,
	    nullptr, &buffer->allocation, IID_ID3D12Resource, &buffer->buffer);
	CHECK(SUCCEEDED(result), "Create buffer failed (HRESULT 0x{:08X})", static_cast<uint32>(result));

	if (desc.access == RHIAccessMode::Read)
		buffer->state = TransferDest;

	return buffer;
}

RHIRef<RHIBufferView> D3D12Device::createBufferView(const RHIBufferViewDesc& desc)
{
	auto normalized = normalizeRHIBufferViewDesc(desc);
	if (normalized.type == RHIBufferViewType::Constant)
		CHECK(normalized.offset % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0 && normalized.size <= 65536,
		    "D3D12 constant buffer views require a 256-byte aligned offset and at most 64 KiB");

	return makeRHIRef<D3D12BufferView>(std::move(normalized));
}

void* D3D12Device::mapBuffer(RHIBuffer* resource, RHIAccessMode mode) const
{
	auto* buffer = dynamic_cast<D3D12Buffer*>(resource);
	CHECK(buffer && !buffer->mapped && mode != RHIAccessMode::None && buffer->desc.access == mode,
	    "D3D12 mapBuffer access must match an unmapped CPU-accessible buffer");

	D3D12_RANGE range{0, mode == RHIAccessMode::Read ? static_cast<SIZE_T>(buffer->desc.size) : 0};
	void* data{};
	const HRESULT result = buffer->buffer->Map(0, &range, &data);
	CHECK(SUCCEEDED(result), "Map buffer failed (HRESULT 0x{:08X})", static_cast<uint32>(result));
	buffer->mapped = true;

	return data;
}

void D3D12Device::unmapBuffer(RHIBuffer* resource) const noexcept
{
	auto* buffer = dynamic_cast<D3D12Buffer*>(resource);
	if (!buffer || !buffer->mapped)
		return;

	D3D12_RANGE range{0, buffer->desc.access == RHIAccessMode::Write ? static_cast<SIZE_T>(buffer->desc.size) : 0};
	buffer->buffer->Unmap(0, &range);
	buffer->mapped = false;
}

void D3D12Device::destroyBuffer(D3D12Buffer* buffer) noexcept
{
	unmapBuffer(buffer);
	buffer->buffer.Reset();
	buffer->allocation.Reset();
}


void D3D12CommandList::useBuffer(RHIBuffer* buffer, RHIResourceState state, std::string_view operation)
{
	CHECK(buffer, "Buffer cannot be null");
	validateRHIBufferState(*buffer, state, operation);
	if (buffer->getDesc().access == RHIAccessMode::None && !buffer_states.contains(buffer))
		transitionBuffer(buffer, buffer->getState(), buffer->getState());
	else
		current_command->trackResource(buffer);
}

void D3D12CommandList::clearBuffer(RHIBuffer* buffer, uint32 value)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	endRendering();
	CHECK(buffer && buffer->getDesc().size % 4 == 0 && buffer->getDesc().access == RHIAccessMode::None, "clearBuffer requires a GPU buffer with a DWORD-aligned size");
	validateRHIBufferState(*buffer, TransferDest, "clearBuffer");

	std::vector<uint32> values(buffer->getDesc().size / 4, value);
	writeBuffer(buffer, 0, values.data(), buffer->getDesc().size);
}

void D3D12CommandList::copyBuffer(RHIBuffer* dst, uint64 dst_offset, RHIBuffer* src, uint64 src_offset, uint64 size)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	endRendering();
	validateRHIBufferCopy(dst, dst_offset, src, src_offset, size);

	useBuffer(src, TransferSource, "copyBuffer source");
	useBuffer(dst, TransferDest, "copyBuffer destination");
	current_command->getHandle()->CopyBufferRegion(static_cast<D3D12Buffer*>(dst)->getHandle(),
	    dst_offset, static_cast<D3D12Buffer*>(src)->getHandle(), src_offset, size);
}

void D3D12CommandList::writeBuffer(RHIBuffer* buffer, uint64 offset, const void* data, uint64 size)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	endRendering();
	if (size == 0)
		return;
	CHECK(buffer && data && buffer->getDesc().access == RHIAccessMode::None &&
	        offset <= buffer->getDesc().size && size <= buffer->getDesc().size - offset,
	    "Invalid GPU buffer upload range");

	auto upload = device.createBuffer(RHIBufferDesc{}
	        .setSize(size)
	        .setUsage(RHIBufferUsage::TransferSource)
	        .setAccess(RHIAccessMode::Write));
	std::memcpy(device.mapBuffer(upload.get(), RHIAccessMode::Write), data, size);
	device.unmapBuffer(upload.get());

	transitionBuffer(upload.get(), upload->getState(), TransferSource);
	transitionBuffer(buffer, buffer->getState(), TransferDest);
	copyBuffer(buffer, offset, upload.get(), 0, size);
}

void D3D12CommandList::transitionBuffer(RHIBuffer* resource, RHIResourceState before, RHIResourceState after)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	auto* buffer = static_cast<D3D12Buffer*>(resource);
	CHECK(buffer && after != Unknown, "Invalid buffer transition");
	validateRHIBufferTransition(*buffer, before, after);

	current_command->trackResource(buffer);
	const auto next = toD3D12ResourceStates(after);
	if (buffer->desc.access == RHIAccessMode::None) {
		auto [it, inserted] = buffer_states.try_emplace(buffer, D3D12_RESOURCE_STATE_COMMON);
		barrier(buffer->getHandle(), it->second, next);
		it->second = next;
	}

	buffer->state = after;
}

}        // namespace Vortex

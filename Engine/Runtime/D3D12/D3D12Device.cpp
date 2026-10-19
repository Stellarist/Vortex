module Runtime.D3D12;

import d3d12;

namespace Vortex {

D3D12Device::D3D12Device(D3D12Context& context, D3D12Ptr<ID3D12Device> native_device) :
    device(std::move(native_device)), context(&context)
{
	const D3D12MA::ALLOCATOR_DESC allocator_desc{
	    .pDevice = device.Get(),
	    .pAdapter = context.getAdapter(),
	};

	const HRESULT allocator_result = D3D12MA::CreateAllocator(&allocator_desc, &allocator);
	CHECK(SUCCEEDED(allocator_result), "Create D3D12 memory allocator failed (HRESULT 0x{:08X})", static_cast<uint32>(allocator_result));

	render_target_descriptors = std::make_unique<D3D12DescriptorAllocator>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 8192);
	depth_stencil_descriptors = std::make_unique<D3D12DescriptorAllocator>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 8192);
	resource_descriptors = std::make_unique<D3D12DescriptorAllocator>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
	sampler_descriptors = std::make_unique<D3D12DescriptorAllocator>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1024);
	resource_heap = std::make_unique<D3D12DescriptorHeap>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 65536, true);
	sampler_heap = std::make_unique<D3D12DescriptorHeap>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048, true);
	queue = std::make_unique<D3D12Queue>(*this);
}

D3D12Device::~D3D12Device() noexcept
{
	const HRESULT result = queue->waitIdle();
	CHECK(SUCCEEDED(result), "D3D12 device shutdown wait failed (HRESULT 0x{:08X})", static_cast<uint32>(result));
	queue.reset();
}

void D3D12Device::waitIdle()
{
	if (queue)
		queue->wait(queue->signal());
}

}        // namespace Vortex

module Runtime.D3D12;

import d3d12;

namespace Vortex {

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 capacity, bool visible) : capacity(capacity)
{
	const D3D12_DESCRIPTOR_HEAP_DESC desc{
	    .Type = type,
	    .NumDescriptors = capacity,
	    .Flags = visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};

	const HRESULT result = device.getHandle()->CreateDescriptorHeap(&desc, IID_ID3D12DescriptorHeap, &heap);
	CHECK(SUCCEEDED(result), "CreateDescriptorHeap failed (HRESULT 0x{:08X})", static_cast<uint32>(result));
	stride = device.getHandle()->GetDescriptorHandleIncrementSize(type);
	free_ranges.reserve(capacity);
}

D3D12DescriptorIndex D3D12DescriptorHeap::allocate(uint32 count)
{
	CHECK(count, "Descriptor allocation requires a nonzero count");
	for (auto it = free_ranges.begin(); it != free_ranges.end(); ++it) {
		if (it->count < count)
			continue;
		const auto first = it->first;
		it->first += count;
		it->count -= count;
		if (!it->count)
			free_ranges.erase(it);
		return first;
	}

	CHECK(count <= capacity - used, "D3D12 descriptor heap has no contiguous range of {} descriptors (capacity {})", count, capacity);
	const auto first = used;
	used += count;
	return first;
}

void D3D12DescriptorHeap::release(D3D12DescriptorIndex index, uint32 count) noexcept
{
	if (index == D3D12InvalidDescriptorIndex || !count)
		return;

	auto it = std::ranges::lower_bound(free_ranges, index, {}, &D3D12DescriptorRange::first);
	it = free_ranges.insert(it, {index, count});
	if (it != free_ranges.begin()) {
		auto previous = std::prev(it);
		if (previous->first + previous->count == it->first) {
			previous->count += it->count;
			it = std::prev(free_ranges.erase(it));
		}
	}

	if (std::next(it) != free_ranges.end() && it->first + it->count == std::next(it)->first) {
		it->count += std::next(it)->count;
		free_ranges.erase(std::next(it));
	}

	if (it->first + it->count == used) {
		used = it->first;
		free_ranges.erase(it);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::cpu(D3D12DescriptorIndex index) const noexcept
{
	return {heap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<SIZE_T>(index) * stride};
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::gpu(D3D12DescriptorIndex index) const noexcept
{
	return {heap->GetGPUDescriptorHandleForHeapStart().ptr + static_cast<UINT64>(index) * stride};
}


D3D12DescriptorAllocator::D3D12DescriptorAllocator(D3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 page_size) :
    device(device), type(type), page_size(page_size)
{
	CHECK(page_size, "Descriptor allocator requires a nonzero page size");
}

D3D12DescriptorIndex D3D12DescriptorAllocator::allocateDescriptor()
{
	for (uint32 index = 0; index < pages.size(); ++index)
		if (pages[index]->hasAvailable())
			return index * page_size + pages[index]->allocate();

	CHECK((uint64(pages.size()) + 1) * page_size <= D3D12InvalidDescriptorIndex,
	    "D3D12 descriptor index space exhausted");
	auto& page = pages.emplace_back(std::make_unique<D3D12DescriptorHeap>(device, type, page_size));
	return static_cast<uint32>(pages.size() - 1) * page_size + page->allocate();
}

void D3D12DescriptorAllocator::releaseDescriptor(D3D12DescriptorIndex index) noexcept
{
	if (index != D3D12InvalidDescriptorIndex)
		pages[index / page_size]->release(index % page_size);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorAllocator::getCpuHandle(D3D12DescriptorIndex index) const noexcept
{
	return index != D3D12InvalidDescriptorIndex ? pages[index / page_size]->cpu(index % page_size) : D3D12_CPU_DESCRIPTOR_HANDLE{};
}


D3D12RootSignature::D3D12RootSignature(ID3D12Device* device, const std::vector<RHIRef<RHIBindingLayout>>& layouts, bool graphics)
{
	std::vector<D3D12_ROOT_PARAMETER> parameters;
	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> ranges(layouts.size() * 2);
	tables.resize(layouts.size());

	uint32 dwords{};
	for (uint32 space = 0; space < layouts.size(); ++space) {
		CHECK(space != D3D12PushConstantSpace, "Register space 255 is reserved for push constants");
		if (const auto* bindless = layouts[space]->getBindlessDesc()) {
			const bool sampler = bindless->type == RHIBindingType::Sampler;
			ranges[space * 2 + sampler].push_back({toD3D12DescriptorRangeType(bindless->type), bindless->max_capacity,
			    bindless->slot, space, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND});
		} else {
			const auto* desc = layouts[space]->getDesc();
			for (const auto& item : desc->bindings) {
				if (item.type == RHIBindingType::PushConstants) {
					CHECK(constants == D3D12InvalidRootParameter && item.size % 4 == 0, "D3D12 supports one DWORD-aligned push constant block");
					constants = static_cast<uint32>(parameters.size());
					constant_size = item.size;
					D3D12_ROOT_PARAMETER parameter{};
					parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
					parameter.Constants = {0, D3D12PushConstantSpace, item.size / 4};
					parameters.push_back(parameter);
					dwords += item.size / 4;
					continue;
				}

				const bool sampler = item.type == RHIBindingType::Sampler;
				ranges[space * 2 + sampler].push_back({toD3D12DescriptorRangeType(item.type), item.array_size, item.slot, space, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND});
			}
		}

		for (uint32 sampler = 0; sampler < 2; ++sampler) {
			const auto& table_ranges = ranges[space * 2 + sampler];
			if (table_ranges.empty())
				continue;

			auto& index = sampler ? tables[space].samplers : tables[space].resources;
			index = static_cast<uint32>(parameters.size());
			D3D12_ROOT_PARAMETER parameter{};
			parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameter.DescriptorTable = {static_cast<UINT>(table_ranges.size()), table_ranges.data()};
			parameters.push_back(parameter);
			++dwords;
		}
	}

	CHECK(dwords <= 64, "D3D12 root signature requires {} DWORDs (maximum 64)", dwords);

	const D3D12_ROOT_SIGNATURE_DESC desc{
	    .NumParameters = static_cast<UINT>(parameters.size()),
	    .pParameters = parameters.data(),
	    .Flags = graphics ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT : D3D12_ROOT_SIGNATURE_FLAG_NONE,
	};

	D3D12Ptr<ID3DBlob> blob, errors;
	const HRESULT result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errors);
	CHECK(SUCCEEDED(result), "D3D12 root signature serialization failed: {}", errors ? static_cast<const char*>(errors->GetBufferPointer()) : "unknown error");

	const HRESULT create_result = device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_ID3D12RootSignature, &handle);
	CHECK(SUCCEEDED(create_result), "CreateRootSignature failed (HRESULT 0x{:08X})", static_cast<uint32>(create_result));
}

D3D12BindingSet::~D3D12BindingSet()
{
	device.destroyBindingSet(this);
}

RHIRef<RHIBindingLayout> D3D12Device::createBindingLayout(const RHIBindingLayoutDesc& desc)
{
	validateRHIBindingLayoutDesc(desc);

	return makeRHIRef<D3D12BindingLayout>(desc);
}

RHIRef<RHIBindingLayout> D3D12Device::createBindlessLayout(const RHIBindlessLayoutDesc& desc)
{
	validateRHIBindlessLayoutDesc(desc);
	D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
	const HRESULT result = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
	CHECK(SUCCEEDED(result) && options.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3,
	    "D3D12 bindless descriptor tables require resource binding tier 3");
	CHECK(desc.max_capacity <= (desc.type == RHIBindingType::Sampler ? 2048u : 65536u),
	    "Bindless layout exceeds the D3D12 shader-visible heap capacity");
	return makeRHIRef<D3D12BindingLayout>(desc);
}

static void writeBufferDescriptor(ID3D12Device* device, const RHIBufferViewDesc& desc,
    RHIBindingType type, D3D12_CPU_DESCRIPTOR_HANDLE destination)
{
	auto* buffer = static_cast<D3D12Buffer*>(desc.buffer.get());
	if (type == RHIBindingType::ConstantBuffer) {
		const auto native = toD3D12ConstantBufferViewDesc(desc, buffer->getHandle()->GetGPUVirtualAddress());
		device->CreateConstantBufferView(&native, destination);
	} else if (toD3D12DescriptorRangeType(type) == D3D12_DESCRIPTOR_RANGE_TYPE_UAV) {
		CHECK(buffer->getDesc().access == RHIAccessMode::None, "Upload/readback buffers cannot be bound as UAVs");
		const auto native = toD3D12UnorderedAccessViewDesc(desc);
		device->CreateUnorderedAccessView(buffer->getHandle(), nullptr, &native, destination);
	} else {
		const auto native = toD3D12ShaderResourceViewDesc(desc);
		device->CreateShaderResourceView(buffer->getHandle(), &native, destination);
	}
}

static void writeTextureDescriptor(ID3D12Device* device, const RHITextureViewDesc& desc,
    D3D12_CPU_DESCRIPTOR_HANDLE destination)
{
	auto* texture = static_cast<D3D12Texture*>(desc.texture.get());
	if (desc.type == RHITextureViewType::ShaderResource) {
		const auto native = toD3D12ShaderResourceViewDesc(desc, texture->getDesc().sample_count);
		device->CreateShaderResourceView(texture->getHandle(), &native, destination);
	} else if (desc.type == RHITextureViewType::UnorderedAccess) {
		const auto native = toD3D12UnorderedAccessViewDesc(desc);
		device->CreateUnorderedAccessView(texture->getHandle(), nullptr, &native, destination);
	} else {
		ERROR(Argument, "Invalid D3D12 shader texture view type");
	}
}

static void writeDescriptor(D3D12Device& device, const RHIBindingSetItem& item, D3D12_CPU_DESCRIPTOR_HANDLE destination)
{
	if (item.type == RHIBindingType::Sampler) {
		auto* sampler = dynamic_cast<D3D12Sampler*>(item.resource.get());
		CHECK(sampler, "D3D12 sampler binding requires a D3D12 sampler");
		const auto native = toD3D12SamplerDesc(sampler->getDesc());
		device.getHandle()->CreateSampler(&native, destination);
	} else if (item.type == RHIBindingType::TextureSRV || item.type == RHIBindingType::TextureUAV) {
		auto* view = dynamic_cast<D3D12TextureView*>(item.resource.get());
		CHECK(view, "D3D12 texture binding requires a D3D12 texture view");
		writeTextureDescriptor(device.getHandle(), view->getDesc(), destination);
	} else {
		auto* view = dynamic_cast<D3D12BufferView*>(item.resource.get());
		CHECK(view, "D3D12 buffer binding requires a D3D12 buffer view");
		writeBufferDescriptor(device.getHandle(), view->getDesc(), item.type, destination);
	}
}

static void writeDescriptors(D3D12Device& device, const D3D12BindingSet& binding_set,
    D3D12DescriptorIndex first_resource, D3D12DescriptorIndex first_sampler)
{
	const auto& desc = *binding_set.getDesc();
	const auto& layout = *binding_set.getLayout()->getDesc();
	D3D12DescriptorIndex resource_index{first_resource}, sampler_index{first_sampler};
	for (const auto& binding : layout.bindings) {
		if (binding.type == RHIBindingType::PushConstants)
			continue;
		const bool sampler = binding.type == RHIBindingType::Sampler;
		auto& heap = sampler ? device.getSamplerHeap() : device.getResourceHeap();
		const auto first = sampler ? sampler_index : resource_index;
		for (const auto& item : desc.bindings) {
			if (item.slot != binding.slot || item.type != binding.type)
				continue;
			writeDescriptor(device, item, heap.cpu(first + item.array_element));
		}
		if (sampler)
			sampler_index += binding.array_size;
		else
			resource_index += binding.array_size;
	}
}

RHIRef<RHIBindingSet> D3D12Device::createBindingSet(const RHIBindingSetDesc& desc, const RHIBindingLayout& layout)
{
	validateRHIBindingSetDesc(desc, layout);
	CHECK(dynamic_cast<const D3D12BindingLayout*>(&layout), "D3D12 bindings require a D3D12 layout");
	auto set = makeRHIRef<D3D12BindingSet>(*this, desc, layout);

	size_t resource_count{}, sampler_count{};
	for (const auto& item : layout.getDesc()->bindings) {
		if (item.type == RHIBindingType::Sampler)
			sampler_count += item.array_size;
		else if (item.type != RHIBindingType::PushConstants)
			resource_count += item.array_size;
	}
	CHECK(resource_count <= 65536 && sampler_count <= 2048,
	    "D3D12 binding set exceeds the shader-visible heap capacity");

	if (resource_count) {
		const auto count = static_cast<uint32>(resource_count);
		set->resources = {resource_heap->allocate(count), count};
	}

	if (sampler_count) {
		const auto count = static_cast<uint32>(sampler_count);
		set->samplers = {sampler_heap->allocate(count), count};
	}

	writeDescriptors(*this, *set, set->resources.first, set->samplers.first);
	return set;
}

RHIRef<RHIDescriptorTable> D3D12Device::createDescriptorTable(const RHIBindingLayout& layout, uint32 capacity)
{
	CHECK(dynamic_cast<const D3D12BindingLayout*>(&layout) && layout.getBindlessDesc(),
	    "D3D12 descriptor tables require a D3D12 bindless layout");
	return makeRHIRef<D3D12DescriptorTable>(*this, layout, capacity);
}

D3D12DescriptorTable::D3D12DescriptorTable(D3D12Device& device, const RHIBindingLayout& layout, uint32 capacity) :
    layout(&layout), device(device)
{
	CHECK(layout.getBindlessDesc(), "Descriptor tables require a bindless layout");
	const auto& binding = *layout.getBindlessDesc();
	CHECK(capacity && capacity <= binding.max_capacity, "Descriptor table capacity must be between 1 and its layout maximum");
	descriptors.resize(binding.max_capacity, D3D12InvalidDescriptorIndex);

	auto& heap = binding.type == RHIBindingType::Sampler ? device.getSamplerHeap() : device.getResourceHeap();
	first_descriptor = heap.allocate(binding.max_capacity);
	this->capacity = capacity;
}

D3D12DescriptorTable::~D3D12DescriptorTable()
{
	const bool sampler = layout->getBindlessDesc()->type == RHIBindingType::Sampler;
	auto& allocator = sampler ? device.getSamplerDescriptors() : device.getResourceDescriptors();
	for (const auto descriptor : descriptors)
		allocator.releaseDescriptor(descriptor);

	auto& heap = sampler ? device.getSamplerHeap() : device.getResourceHeap();
	heap.release(first_descriptor, layout->getBindlessDesc()->max_capacity);
}

void D3D12DescriptorTable::write(uint32 index, RHIResource* resource)
{
	CHECK(index < capacity, "Descriptor table index {} exceeds capacity {}", index, capacity);
	const auto& binding = *layout->getBindlessDesc();
	RHIBindingSetItem item{.resource = resource, .type = binding.type, .slot = binding.slot, .array_element = index};
	validateRHIBindingSetItem(item, binding);

	const bool sampler = binding.type == RHIBindingType::Sampler;
	auto& allocator = sampler ? device.getSamplerDescriptors() : device.getResourceDescriptors();
	auto& heap = sampler ? device.getSamplerHeap() : device.getResourceHeap();
	if (descriptors[index] == D3D12InvalidDescriptorIndex)
		descriptors[index] = allocator.allocateDescriptor();

	const auto source = allocator.getCpuHandle(descriptors[index]);
	writeDescriptor(device, item, source);
	device.getHandle()->CopyDescriptorsSimple(1, heap.cpu(first_descriptor + index), source,
	    sampler ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3D12DescriptorTable::clear(uint32 index)
{
	CHECK(index < capacity, "Descriptor table index {} exceeds capacity {}", index, capacity);
	auto& allocator = layout->getBindlessDesc()->type == RHIBindingType::Sampler ? device.getSamplerDescriptors() : device.getResourceDescriptors();
	allocator.releaseDescriptor(std::exchange(descriptors[index], D3D12InvalidDescriptorIndex));
}

void D3D12DescriptorTable::resize(uint32 new_capacity, bool keep_contents)
{
	CHECK(new_capacity && new_capacity <= layout->getBindlessDesc()->max_capacity,
	    "Descriptor table capacity must be between 1 and its layout maximum");
	for (uint32 i = keep_contents ? new_capacity : 0; i < capacity; ++i)
		clear(i);
	capacity = new_capacity;
}

void D3D12Device::destroyBindingSet(D3D12BindingSet* set) noexcept
{
	const auto resources = std::exchange(set->resources, D3D12DescriptorRange{});
	const auto samplers = std::exchange(set->samplers, D3D12DescriptorRange{});
	resource_heap->release(resources.first, resources.count);
	sampler_heap->release(samplers.first, samplers.count);
}

}        // namespace Vortex

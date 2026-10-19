export module Runtime.D3D12:Binding;

import d3d12;
import Core;
import Runtime.RHI;
import :Types;

export namespace Vortex {

class D3D12Device;

using D3D12DescriptorIndex = uint32;
inline constexpr D3D12DescriptorIndex D3D12InvalidDescriptorIndex = ~0u;

struct D3D12DescriptorRange {
	D3D12DescriptorIndex first{D3D12InvalidDescriptorIndex};
	uint32 count{};
};

struct D3D12RootSignature {
	struct Table {
		uint32 resources{D3D12InvalidRootParameter};
		uint32 samplers{D3D12InvalidRootParameter};
	};

	D3D12Ptr<ID3D12RootSignature> handle{};
	std::vector<Table> tables{};

	uint32 constants{D3D12InvalidRootParameter};
	uint32 constant_size{};

	D3D12RootSignature(ID3D12Device* device, const std::vector<RHIRef<RHIBindingLayout>>& layouts, bool graphics);
};


class D3D12BindingLayout : public RHIBindingLayout {
private:
	std::variant<RHIBindingLayoutDesc, RHIBindlessLayoutDesc> desc{};

public:
	D3D12BindingLayout(RHIBindingLayoutDesc desc) : desc(std::move(desc)) {}
	D3D12BindingLayout(RHIBindlessLayoutDesc desc) : desc(desc) {}

	const RHIBindingLayoutDesc* getDesc() const noexcept override { return std::get_if<RHIBindingLayoutDesc>(&desc); }
	const RHIBindlessLayoutDesc* getBindlessDesc() const noexcept override { return std::get_if<RHIBindlessLayoutDesc>(&desc); }
};


class D3D12BindingSet : public RHIBindingSet {
private:
	RHIBindingSetDesc desc{};

	RHIRef<const RHIBindingLayout> layout{};

	D3D12DescriptorRange resources{};
	D3D12DescriptorRange samplers{};

	D3D12Device& device;

	friend class D3D12Device;
	friend class D3D12CommandList;

public:
	D3D12BindingSet(D3D12Device& device, RHIBindingSetDesc desc, const RHIBindingLayout& layout) :
	    desc(std::move(desc)), layout(&layout), device(device) {}
	~D3D12BindingSet() override;

	const RHIBindingSetDesc* getDesc() const noexcept override { return &desc; }
	const RHIBindingLayout* getLayout() const noexcept override { return layout.get(); }
};


class D3D12DescriptorHeap {
private:
	D3D12Ptr<ID3D12DescriptorHeap> heap{};
	std::vector<D3D12DescriptorRange> free_ranges{};

	uint32 stride{};
	uint32 capacity{};
	uint32 used{};

public:
	D3D12DescriptorHeap(D3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 capacity, bool visible = false);

	D3D12DescriptorIndex allocate(uint32 count = 1);
	void release(D3D12DescriptorIndex index, uint32 count = 1) noexcept;
	bool hasAvailable() const noexcept { return used < capacity || !free_ranges.empty(); }

	D3D12_CPU_DESCRIPTOR_HANDLE cpu(D3D12DescriptorIndex index) const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu(D3D12DescriptorIndex index) const noexcept;

	ID3D12DescriptorHeap* getHandle() const noexcept { return heap.Get(); }
};


class D3D12DescriptorAllocator {
private:
	std::vector<std::unique_ptr<D3D12DescriptorHeap>> pages{};
	D3D12_DESCRIPTOR_HEAP_TYPE type{};
	uint32 page_size{};

	D3D12Device& device;

public:
	D3D12DescriptorAllocator(D3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 page_size);

	D3D12DescriptorIndex allocateDescriptor();
	void releaseDescriptor(D3D12DescriptorIndex index) noexcept;

	D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(D3D12DescriptorIndex index) const noexcept;
};


class D3D12DescriptorTable : public RHIDescriptorTable {
private:
	std::vector<D3D12DescriptorIndex> descriptors{};
	RHIRef<const RHIBindingLayout> layout{};

	D3D12DescriptorIndex first_descriptor{D3D12InvalidDescriptorIndex};
	uint32 capacity{};

	D3D12Device& device;

	friend class D3D12CommandList;

public:
	D3D12DescriptorTable(D3D12Device& device, const RHIBindingLayout& layout, uint32 capacity);
	~D3D12DescriptorTable() override;

	void write(uint32 index, RHIResource* resource) override;
	void clear(uint32 index) override;
	void resize(uint32 capacity, bool keep_contents = true) override;

	const RHIBindingSetDesc* getDesc() const noexcept override { return nullptr; }
	const RHIBindingLayout* getLayout() const noexcept override { return layout.get(); }
	uint32 getCapacity() const noexcept override { return capacity; }
};

}        // namespace Vortex

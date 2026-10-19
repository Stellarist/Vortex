export module Runtime.D3D12:Buffer;

import d3d12;
import Core;
import :Types;
import :Device;
import Runtime.RHI;

export namespace Vortex {

class D3D12Buffer : public RHIBuffer {
private:
	RHIBufferDesc desc{};
	RHIResourceState state{Unknown};

	D3D12Ptr<ID3D12Resource> buffer{};
	D3D12Ptr<D3D12MA::Allocation> allocation{};
	bool mapped{};

	D3D12Device& device;

	friend class D3D12Device;
	friend class D3D12CommandList;

protected:
	void applyName(const std::string& name) noexcept override
	{
		setD3D12Name(buffer.Get(), name);
	}

public:
	D3D12Buffer(D3D12Device& device, RHIBufferDesc desc) : desc(std::move(desc)), device(device) {}
	~D3D12Buffer() override { device.destroyBuffer(this); }

	const RHIBufferDesc& getDesc() const noexcept override { return desc; }
	RHIResourceState getState() const noexcept override { return state; }

	ID3D12Resource* getHandle() const noexcept { return buffer.Get(); }
};


class D3D12BufferView : public RHIBufferView {
private:
	RHIBufferViewDesc desc{};

public:
	D3D12BufferView(RHIBufferViewDesc desc) : desc(std::move(desc)) {}

	const RHIBufferViewDesc& getDesc() const noexcept override { return desc; }
	RHIBuffer& getBuffer() const noexcept override { return *desc.buffer; }
};

}        // namespace Vortex

export module Runtime.D3D12:Texture;

import d3d12;
import Core;
import Runtime.RHI;
import :Types;
import :Binding;
import :Device;

export namespace Vortex {

class D3D12Texture : public RHITexture {
private:
	RHITextureDesc desc{};
	RHIResourceState state{Common};

	D3D12Ptr<ID3D12Resource> texture{};
	D3D12Ptr<D3D12MA::Allocation> allocation{};

	D3D12Device& device;

	friend class D3D12Device;
	friend class D3D12CommandList;

protected:
	void applyName(const std::string& name) noexcept override
	{
		setD3D12Name(texture.Get(), name);
	}

public:
	D3D12Texture(D3D12Device& device, RHITextureDesc desc) : desc(std::move(desc)), device(device) {}
	~D3D12Texture() override { device.destroyTexture(this); }

	const RHITextureDesc& getDesc() const noexcept override { return desc; }
	RHIResourceState getState() const noexcept override { return state; }

	ID3D12Resource* getHandle() const noexcept { return texture.Get(); }
};


class D3D12TextureView : public RHITextureView {
private:
	RHITextureViewDesc desc{};

	D3D12DescriptorIndex descriptor{D3D12InvalidDescriptorIndex};

	D3D12Device& device;

	friend class D3D12Device;

public:
	D3D12TextureView(D3D12Device& device, RHITextureViewDesc desc) : desc(std::move(desc)), device(device) {}
	~D3D12TextureView() override { device.destroyTextureView(this); }

	const RHITextureViewDesc& getDesc() const noexcept override { return desc; }
	RHITexture& getTexture() const noexcept override { return *desc.texture; }

	D3D12_CPU_DESCRIPTOR_HANDLE getHandle() const noexcept;
};


class D3D12StagingTexture : public RHIStagingTexture {
private:
	RHITextureDesc desc{};
	RHIAccessMode access{};

	D3D12Ptr<ID3D12Resource> buffer{};
	D3D12Ptr<D3D12MA::Allocation> allocation{};
	uint64 size{};
	bool mapped{};

	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints{};

	D3D12Device& device;

	friend class D3D12Device;
	friend class D3D12CommandList;

protected:
	void applyName(const std::string& name) noexcept override
	{
		setD3D12Name(buffer.Get(), name);
	}

public:
	D3D12StagingTexture(D3D12Device& device, RHITextureDesc desc, RHIAccessMode access) : desc(std::move(desc)), access(access), device(device) {}
	~D3D12StagingTexture() override { device.destroyStagingTexture(this); }

	const RHITextureDesc& getDesc() const noexcept override { return desc; }
	RHIAccessMode getAccess() const noexcept override { return access; }

	ID3D12Resource* getHandle() const noexcept { return buffer.Get(); }
};


class D3D12Sampler : public RHISampler {
private:
	RHISamplerDesc desc{};

public:
	D3D12Sampler(RHISamplerDesc desc) : desc(std::move(desc)) {}

	const RHISamplerDesc& getDesc() const noexcept override { return desc; }
};

}        // namespace Vortex

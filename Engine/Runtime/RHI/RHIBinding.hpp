export module Runtime.RHI:Binding;

import Core;
import :Buffer;
import :Texture;

export namespace Vortex {

struct RHIBindingLayoutItem {
	RHIBindingType type{RHIBindingType::None};

	uint32 slot{};
	uint32 size{};

private:
	static RHIBindingLayoutItem make(RHIBindingType type, uint32 slot) noexcept
	{
		RHIBindingLayoutItem item{};
		item.type = type;
		item.slot = slot;
		return item;
	}

public:
	static RHIBindingLayoutItem textureSRV(uint32 slot) noexcept
	{ return make(RHIBindingType::TextureSRV, slot); }

	static RHIBindingLayoutItem textureUAV(uint32 slot) noexcept
	{ return make(RHIBindingType::TextureUAV, slot); }

	static RHIBindingLayoutItem typedBufferSRV(uint32 slot) noexcept
	{ return make(RHIBindingType::TypedBufferSRV, slot); }

	static RHIBindingLayoutItem typedBufferUAV(uint32 slot) noexcept
	{ return make(RHIBindingType::TypedBufferUAV, slot); }

	static RHIBindingLayoutItem structuredBufferSRV(uint32 slot) noexcept
	{ return make(RHIBindingType::StructuredBufferSRV, slot); }

	static RHIBindingLayoutItem structuredBufferUAV(uint32 slot) noexcept
	{ return make(RHIBindingType::StructuredBufferUAV, slot); }

	static RHIBindingLayoutItem rawBufferSRV(uint32 slot) noexcept
	{ return make(RHIBindingType::RawBufferSRV, slot); }

	static RHIBindingLayoutItem rawBufferUAV(uint32 slot) noexcept
	{ return make(RHIBindingType::RawBufferUAV, slot); }

	static RHIBindingLayoutItem constantBuffer(uint32 slot) noexcept
	{ return make(RHIBindingType::ConstantBuffer, slot); }

	static RHIBindingLayoutItem sampler(uint32 slot) noexcept
	{ return make(RHIBindingType::Sampler, slot); }

	static RHIBindingLayoutItem pushConstants(uint32 size) noexcept
	{
		auto item = make(RHIBindingType::PushConstants, 0);
		item.size = size;
		return item;
	}
};

struct RHIBindingLayoutDesc {
	RHIShaderType visibility{RHIShaderType::None};

	std::vector<RHIBindingLayoutItem> bindings{};

	RHIBindingLayoutDesc& setVisibility(RHIShaderType new_visibility) noexcept
	{
		visibility = new_visibility;
		return *this;
	}

	RHIBindingLayoutDesc& addItem(const RHIBindingLayoutItem& new_item)
	{
		bindings.push_back(new_item);
		return *this;
	}
};

class RHIBindingLayout : public RHIResource {
public:
	virtual const RHIBindingLayoutDesc& getDesc() const noexcept = 0;
};


struct RHIBindingSetItem {
	RHIRef<RHIResource> resource{};

	RHIBindingType type{RHIBindingType::None};

	uint32 slot{};

private:
	static RHIBindingSetItem make(RHIBindingType type, uint32 slot, RHIResource* resource) noexcept
	{
		RHIBindingSetItem item{};
		item.resource = resource;
		item.type = type;
		item.slot = slot;
		return item;
	}

public:
	static RHIBindingSetItem textureSRV(uint32 slot, RHITextureView* view) noexcept
	{ return make(RHIBindingType::TextureSRV, slot, view); }

	static RHIBindingSetItem textureUAV(uint32 slot, RHITextureView* view) noexcept
	{ return make(RHIBindingType::TextureUAV, slot, view); }

	static RHIBindingSetItem typedBufferSRV(uint32 slot, RHIBufferView* view) noexcept
	{ return make(RHIBindingType::TypedBufferSRV, slot, view); }

	static RHIBindingSetItem typedBufferUAV(uint32 slot, RHIBufferView* view) noexcept
	{ return make(RHIBindingType::TypedBufferUAV, slot, view); }

	static RHIBindingSetItem structuredBufferSRV(uint32 slot, RHIBufferView* view) noexcept
	{ return make(RHIBindingType::StructuredBufferSRV, slot, view); }

	static RHIBindingSetItem structuredBufferUAV(uint32 slot, RHIBufferView* view) noexcept
	{ return make(RHIBindingType::StructuredBufferUAV, slot, view); }

	static RHIBindingSetItem rawBufferSRV(uint32 slot, RHIBufferView* view) noexcept
	{ return make(RHIBindingType::RawBufferSRV, slot, view); }

	static RHIBindingSetItem rawBufferUAV(uint32 slot, RHIBufferView* view) noexcept
	{ return make(RHIBindingType::RawBufferUAV, slot, view); }

	static RHIBindingSetItem constantBuffer(uint32 slot, RHIBufferView* view) noexcept
	{ return make(RHIBindingType::ConstantBuffer, slot, view); }

	static RHIBindingSetItem sampler(uint32 slot, RHISampler* sampler) noexcept
	{ return make(RHIBindingType::Sampler, slot, sampler); }
};

struct RHIBindingSetDesc {
	std::vector<RHIBindingSetItem> bindings{};

	RHIBindingSetDesc& addItem(const RHIBindingSetItem& new_item)
	{
		bindings.push_back(new_item);
		return *this;
	}
};

class RHIBindingSet : public RHIResource {
public:
	virtual const RHIBindingSetDesc& getDesc() const noexcept = 0;
	virtual const RHIBindingLayout* getLayout() const noexcept = 0;
};

}        // namespace Vortex

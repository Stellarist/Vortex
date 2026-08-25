export module Runtime.Graphics:RHI.Binding;

import Core;
import :RHI.Sampler;
import :RHI.Buffer;
import :RHI.Texture;

export namespace Vortex {

struct RHIBindingLayoutItem {
	RHIBindingType type{RHIBindingType::None};

	uint32 slot{};
	uint32 count{1};
	uint32 size{};

	RHIBindingLayoutItem& setSlot(uint32 new_slot) noexcept
	{
		slot = new_slot;
		return *this;
	}

	RHIBindingLayoutItem& setCount(uint32 new_count) noexcept
	{
		count = new_count;
		return *this;
	}

	RHIBindingLayoutItem& setType(RHIBindingType new_type) noexcept
	{
		type = new_type;
		return *this;
	}

	RHIBindingLayoutItem& setPushConstants(uint32 new_size, uint32 new_slot = 0) noexcept
	{
		type = RHIBindingType::PushConstants;
		slot = new_slot;
		count = 1;
		size = new_size;
		return *this;
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

	RHIBindingLayoutDesc& setItems(const std::vector<RHIBindingLayoutItem>& new_bindings)
	{
		bindings = new_bindings;
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
	uint32         slot{};

	RHIBindingSetItem& setType(RHIBindingType new_type) noexcept
	{
		type = new_type;
		return *this;
	}

	RHIBindingSetItem& setSlot(uint32 new_slot) noexcept
	{
		slot = new_slot;
		return *this;
	}

	RHIBindingSetItem& setTextureView(uint32 new_slot, RHITextureView* new_texture_view, RHIBindingType new_type = RHIBindingType::TextureSRV) noexcept
	{
		slot = new_slot;
		type = new_type;
		resource = new_texture_view;
		return *this;
	}

	RHIBindingSetItem& setSampler(uint32 new_slot, RHISampler* new_sampler) noexcept
	{
		slot = new_slot;
		type = RHIBindingType::Sampler;
		resource = new_sampler;
		return *this;
	}

	RHIBindingSetItem& setBufferView(uint32 new_slot, RHIBufferView* new_buffer_view, RHIBindingType new_type = RHIBindingType::ConstantBuffer) noexcept
	{
		slot = new_slot;
		type = new_type;
		resource = new_buffer_view;
		return *this;
	}

	RHIBindingSetItem& setPushConstants(uint32 new_slot = 0) noexcept
	{
		slot = new_slot;
		type = RHIBindingType::PushConstants;
		resource = nullptr;
		return *this;
	}
};

struct RHIBindingSetDesc {
	std::vector<RHIBindingSetItem> bindings{};

	RHIBindingSetDesc& addItem(const RHIBindingSetItem& new_item)
	{
		bindings.push_back(new_item);
		return *this;
	}

	RHIBindingSetDesc& setItems(const std::vector<RHIBindingSetItem>& new_bindings)
	{
		bindings = new_bindings;
		return *this;
	}
};

class RHIBindingSet : public RHIResource {
public:
	virtual const RHIBindingSetDesc& getDesc() const noexcept = 0;
	virtual const RHIBindingLayout*  getLayout() const noexcept = 0;
};

}        // namespace Vortex

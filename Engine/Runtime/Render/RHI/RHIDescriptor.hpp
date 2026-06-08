#pragma once

#include "RHIResources.hpp"

// descriptor layout
struct RHIDescriptorLayoutItem {
	RHIDescriptorType type{RHIDescriptorType::None};

	uint32_t slot{};
	uint32_t count{1};

	RHIDescriptorLayoutItem& setSlot(uint32_t new_slot)
	{
		slot = new_slot;
		return *this;
	}

	RHIDescriptorLayoutItem& setCount(uint32_t new_count)
	{
		count = new_count;
		return *this;
	}

	RHIDescriptorLayoutItem& setType(RHIDescriptorType new_type)
	{
		type = new_type;
		return *this;
	}
};

struct RHIDescriptorLayoutDesc {
	RHIShaderType visibility{RHIShaderType::None};

	std::vector<RHIDescriptorLayoutItem> bindings;

	RHIDescriptorLayoutDesc& setVisibility(RHIShaderType new_visibility)
	{
		visibility = new_visibility;
		return *this;
	}

	RHIDescriptorLayoutDesc& addBindingItem(const RHIDescriptorLayoutItem& new_item)
	{
		bindings.push_back(new_item);
		return *this;
	}

	RHIDescriptorLayoutDesc& setBindings(const std::vector<RHIDescriptorLayoutItem>& new_bindings)
	{
		bindings = new_bindings;
		return *this;
	}
};

class RHIDescriptorLayout : public RHIResource {
public:
	virtual const RHIDescriptorLayoutDesc& getDesc() const = 0;
};


// descriptor set
struct RHIDescriptorSetItem {
	RHIResource* resource{};

	uint32_t slot{};

	RHIDescriptorType type{RHIDescriptorType::None};

	RHIDescriptorSetItem& setSlot(uint32_t new_slot)
	{
		slot = new_slot;
		return *this;
	}

	RHIDescriptorSetItem& setType(RHIDescriptorType new_type)
	{
		type = new_type;
		return *this;
	}

	RHIDescriptorSetItem& setTexture(uint32_t new_slot, RHITexture* new_texture, RHIDescriptorType new_type = RHIDescriptorType::TextureSRV)
	{
		slot = new_slot;
		type = new_type;
		resource = new_texture;
		return *this;
	}

	RHIDescriptorSetItem& setSampler(uint32_t new_slot, RHISampler* new_sampler, RHIDescriptorType new_type = RHIDescriptorType::Sampler)
	{
		slot = new_slot;
		type = new_type;
		resource = new_sampler;
		return *this;
	}

	RHIDescriptorSetItem& setBuffer(uint32_t new_slot, RHIBuffer* new_buffer, RHIDescriptorType new_type = RHIDescriptorType::UniformBuffer)
	{
		slot = new_slot;
		type = new_type;
		resource = new_buffer;
		return *this;
	}
};

struct RHIDescriptorSetDesc {
	std::vector<RHIDescriptorSetItem> bindings;

	RHIDescriptorSetDesc& addItem(const RHIDescriptorSetItem& new_item)
	{
		bindings.push_back(new_item);
		return *this;
	}

	RHIDescriptorSetDesc& setItems(const std::vector<RHIDescriptorSetItem>& new_bindings)
	{
		bindings = new_bindings;
		return *this;
	}
};

class RHIDescriptorSet : public RHIResource {
public:
	virtual const RHIDescriptorSetDesc& getDesc() const = 0;
	virtual const RHIDescriptorLayout*  getLayout() const = 0;
};

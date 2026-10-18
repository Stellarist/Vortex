export module Runtime.RDG:Binding;

import Core;
import Runtime.RHI;
import :Pass;

export namespace Vortex {

class RDGBindingSet {
private:
	struct Item {
		RHIBindingType type{};
		uint32 slot{};
		std::variant<RDGBufferViewRef, RDGTextureViewRef, RHIRef<RHISampler>> resource{};
	};

	RHIRef<RHIBindingLayout> layout{};
	std::vector<Item> items{};

	friend class RDGBindingBuilder;

public:
	RHIRef<RHIBindingSet> resolve(RDGPassContext& context) const;
};


class RDGBindingBuilder {
private:
	RDGPassBuilder* pass{};
	std::string name{};
	RDGBindingSet bindings{};

	void validateSlot(uint32 slot, RHIBindingType type) const;

	RDGBindingBuilder& addBuffer(uint32 slot, RDGBufferRef buffer, RHIBufferViewDesc desc,
	    RHIBindingType type, RDGAccess access, RHIResourceState state);
	RDGBindingBuilder& addTexture(uint32 slot, RDGTextureRef texture, RHITextureViewDesc desc,
	    RHIBindingType type, RDGAccess access, RHIResourceState state);

public:
	RDGBindingBuilder(RDGPassBuilder& pass, RHIBindingLayout& layout, std::string_view name = "Bindings");

	RDGBindingBuilder& constantBuffer(uint32 slot, RDGBufferRef buffer, RHIBufferViewDesc desc = {});
	RDGBindingBuilder& structuredBufferSRV(uint32 slot, RDGBufferRef buffer, RHIBufferViewDesc desc = {});
	RDGBindingBuilder& typedBufferSRV(uint32 slot, RDGBufferRef buffer, RHIBufferViewDesc desc);
	RDGBindingBuilder& rawBufferSRV(uint32 slot, RDGBufferRef buffer, RHIBufferViewDesc desc = {});
	RDGBindingBuilder& textureSRV(uint32 slot, RDGTextureRef texture, RHITextureViewDesc desc = {});

	RDGBindingBuilder& structuredBufferUAV(uint32 slot, RDGBufferRef buffer, RDGAccess access, RHIBufferViewDesc desc = {});
	RDGBindingBuilder& typedBufferUAV(uint32 slot, RDGBufferRef buffer, RDGAccess access, RHIBufferViewDesc desc);
	RDGBindingBuilder& rawBufferUAV(uint32 slot, RDGBufferRef buffer, RDGAccess access, RHIBufferViewDesc desc = {});
	RDGBindingBuilder& textureUAV(uint32 slot, RDGTextureRef texture, RDGAccess access, RHITextureViewDesc desc = {});
	RDGBindingBuilder& sampler(uint32 slot, RHISampler& sampler);

	RDGBindingSet build();
};

}        // namespace Vortex

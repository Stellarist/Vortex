module Runtime.RDG;

namespace Vortex {

RHIRef<RHIBindingSet> RDGBindingSet::resolve(RDGPassContext& context) const
{
	CHECK(layout, "Cannot resolve an empty RDG binding set");
	RHIBindingSetDesc desc{};
	for (const auto& item : items) {
		switch (item.type) {
		case RHIBindingType::ConstantBuffer:
			desc.addItem(RHIBindingSetItem::constantBuffer(item.slot, &context.getBufferView(std::get<RDGBufferViewRef>(item.resource))));
			break;
		case RHIBindingType::StructuredBufferSRV:
			desc.addItem(RHIBindingSetItem::structuredBufferSRV(item.slot, &context.getBufferView(std::get<RDGBufferViewRef>(item.resource))));
			break;
		case RHIBindingType::StructuredBufferUAV:
			desc.addItem(RHIBindingSetItem::structuredBufferUAV(item.slot, &context.getBufferView(std::get<RDGBufferViewRef>(item.resource))));
			break;
		case RHIBindingType::TypedBufferSRV:
			desc.addItem(RHIBindingSetItem::typedBufferSRV(item.slot, &context.getBufferView(std::get<RDGBufferViewRef>(item.resource))));
			break;
		case RHIBindingType::TypedBufferUAV:
			desc.addItem(RHIBindingSetItem::typedBufferUAV(item.slot, &context.getBufferView(std::get<RDGBufferViewRef>(item.resource))));
			break;
		case RHIBindingType::RawBufferSRV:
			desc.addItem(RHIBindingSetItem::rawBufferSRV(item.slot, &context.getBufferView(std::get<RDGBufferViewRef>(item.resource))));
			break;
		case RHIBindingType::RawBufferUAV:
			desc.addItem(RHIBindingSetItem::rawBufferUAV(item.slot, &context.getBufferView(std::get<RDGBufferViewRef>(item.resource))));
			break;
		case RHIBindingType::TextureSRV:
			desc.addItem(RHIBindingSetItem::textureSRV(item.slot, &context.getTextureView(std::get<RDGTextureViewRef>(item.resource))));
			break;
		case RHIBindingType::TextureUAV:
			desc.addItem(RHIBindingSetItem::textureUAV(item.slot, &context.getTextureView(std::get<RDGTextureViewRef>(item.resource))));
			break;
		case RHIBindingType::Sampler:
			desc.addItem(RHIBindingSetItem::sampler(item.slot, std::get<RHIRef<RHISampler>>(item.resource).get()));
			break;
		default:
			CHECK(false, "Unsupported RDG binding type");
		}
	}
	return context.getDevice().createBindingSet(desc, *layout);
}

RDGBindingBuilder::RDGBindingBuilder(RDGPassBuilder& pass, RHIBindingLayout& layout, std::string_view name) :
    pass(&pass), name(name)
{
	CHECK(layout.getDesc(),
	    "RDG binding builders require an ordinary binding layout; use a descriptor table and declare its resource accesses explicitly for bindless layouts");
	bindings.layout = &layout;
}

void RDGBindingBuilder::validateSlot(uint32 slot, RHIBindingType type) const
{
	CHECK(bindings.layout, "Cannot reuse an RDG binding builder after build");
	CHECK(Argument, std::ranges::none_of(bindings.items, [slot](const auto& item) { return item.slot == slot; }),
	    "RDG binding slot {} is already assigned", slot);
	CHECK(Argument, std::ranges::any_of(bindings.layout->getDesc()->bindings, [slot, type](const auto& item) { return item.slot == slot && item.type == type; }),
	    "RDG binding slot {} does not match its layout", slot);
}

static void validateBindingAccess(RDGAccess access)
{
	CHECK(Argument, access == RDGAccess::Read || access == RDGAccess::Write || access == RDGAccess::ReadWrite,
	    "Invalid RDG binding access mode");
}

template <typename View>
static void declareBindingAccess(RDGPassBuilder& pass, View view, RDGAccess access, RHIResourceState state)
{
	switch (access) {
	case RDGAccess::Read:
		pass.read(view, state);
		break;
	case RDGAccess::Write:
		pass.write(view, state);
		break;
	case RDGAccess::ReadWrite:
		pass.readWrite(view, state);
		break;
	}
}

RDGBindingBuilder& RDGBindingBuilder::addBuffer(uint32 slot, RDGBufferRef buffer, RHIBufferViewDesc desc,
    RHIBindingType type, RDGAccess access, RHIResourceState state)
{
	validateSlot(slot, type);
	validateBindingAccess(access);

	auto view = pass->createBufferView(std::format("{}.{}", name, slot), buffer, desc);
	declareBindingAccess(*pass, view, access, state);
	bindings.items.push_back({.type = type, .slot = slot, .resource = view});
	return *this;
}

RDGBindingBuilder& RDGBindingBuilder::addTexture(uint32 slot, RDGTextureRef texture, RHITextureViewDesc desc,
    RHIBindingType type, RDGAccess access, RHIResourceState state)
{
	validateSlot(slot, type);
	validateBindingAccess(access);

	auto view = pass->createTextureView(std::format("{}.{}", name, slot), texture, desc);
	declareBindingAccess(*pass, view, access, state);
	bindings.items.push_back({.type = type, .slot = slot, .resource = view});
	return *this;
}

RDGBindingBuilder& RDGBindingBuilder::constantBuffer(uint32 slot, RDGBufferRef buffer, RHIBufferViewDesc desc)
{
	return addBuffer(slot, buffer, desc.setType(RHIBufferViewType::Constant),
	    RHIBindingType::ConstantBuffer, RDGAccess::Read, ConstantBuffer);
}

RDGBindingBuilder& RDGBindingBuilder::structuredBufferSRV(uint32 slot, RDGBufferRef buffer, RHIBufferViewDesc desc)
{
	return addBuffer(slot, buffer, desc.setType(RHIBufferViewType::Structured),
	    RHIBindingType::StructuredBufferSRV, RDGAccess::Read, ShaderResource);
}

RDGBindingBuilder& RDGBindingBuilder::structuredBufferUAV(uint32 slot, RDGBufferRef buffer, RDGAccess access, RHIBufferViewDesc desc)
{
	return addBuffer(slot, buffer, desc.setType(RHIBufferViewType::Structured),
	    RHIBindingType::StructuredBufferUAV, access, UnorderedAccess);
}

RDGBindingBuilder& RDGBindingBuilder::typedBufferSRV(uint32 slot, RDGBufferRef buffer, RHIBufferViewDesc desc)
{
	return addBuffer(slot, buffer, desc.setType(RHIBufferViewType::Typed),
	    RHIBindingType::TypedBufferSRV, RDGAccess::Read, ShaderResource);
}

RDGBindingBuilder& RDGBindingBuilder::typedBufferUAV(uint32 slot, RDGBufferRef buffer, RDGAccess access, RHIBufferViewDesc desc)
{
	return addBuffer(slot, buffer, desc.setType(RHIBufferViewType::Typed),
	    RHIBindingType::TypedBufferUAV, access, UnorderedAccess);
}

RDGBindingBuilder& RDGBindingBuilder::rawBufferSRV(uint32 slot, RDGBufferRef buffer, RHIBufferViewDesc desc)
{
	return addBuffer(slot, buffer, desc.setType(RHIBufferViewType::Raw),
	    RHIBindingType::RawBufferSRV, RDGAccess::Read, ShaderResource);
}

RDGBindingBuilder& RDGBindingBuilder::rawBufferUAV(uint32 slot, RDGBufferRef buffer, RDGAccess access, RHIBufferViewDesc desc)
{
	return addBuffer(slot, buffer, desc.setType(RHIBufferViewType::Raw),
	    RHIBindingType::RawBufferUAV, access, UnorderedAccess);
}

RDGBindingBuilder& RDGBindingBuilder::textureSRV(uint32 slot, RDGTextureRef texture, RHITextureViewDesc desc)
{
	return addTexture(slot, texture, desc.setType(RHITextureViewType::ShaderResource),
	    RHIBindingType::TextureSRV, RDGAccess::Read, ShaderResource);
}

RDGBindingBuilder& RDGBindingBuilder::textureUAV(uint32 slot, RDGTextureRef texture, RDGAccess access, RHITextureViewDesc desc)
{
	CHECK(Argument, desc.subresource.level_count <= 1, "A texture UAV must select exactly one mip level");
	desc.subresource.level_count = 1;
	return addTexture(slot, texture, desc.setType(RHITextureViewType::UnorderedAccess),
	    RHIBindingType::TextureUAV, access, UnorderedAccess);
}

RDGBindingBuilder& RDGBindingBuilder::sampler(uint32 slot, RHISampler& sampler)
{
	validateSlot(slot, RHIBindingType::Sampler);
	bindings.items.push_back({.type = RHIBindingType::Sampler, .slot = slot, .resource = RHIRef<RHISampler>{&sampler}});
	return *this;
}

RDGBindingSet RDGBindingBuilder::build()
{
	CHECK(bindings.layout, "Cannot reuse an RDG binding builder after build");
	const auto count = std::ranges::count_if(bindings.layout->getDesc()->bindings,
	    [](const auto& item) { return item.type != RHIBindingType::PushConstants; });
	CHECK(Argument, bindings.items.size() == static_cast<size_t>(count), "RDG bindings do not cover the layout");
	return std::move(bindings);
}

}        // namespace Vortex

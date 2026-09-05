module Runtime.Graphics;

namespace Vortex {

static bool hasTextureUsage(const RHITextureDesc& desc, RHITextureUsage usage) noexcept
{
	return (desc.usage & usage) != RHITextureUsage::None;
}

static bool hasBufferUsage(const RHIBufferDesc& desc, RHIBufferUsage usage) noexcept
{
	return (desc.usage & usage) != RHIBufferUsage::None;
}

static bool isSingleState(RHIResourceState state) noexcept
{
	const auto bits = static_cast<uint16>(state);
	return bits != 0 && (bits & (bits - 1)) == 0;
}

static bool stateCanRead(RHIResourceState state) noexcept
{
	return state != CopyDest && state != Present;
}

static bool stateCanWrite(RHIResourceState state) noexcept
{
	return state == Common || state == RenderTarget || state == DepthWrite ||
	    state == UnorderedAccess || state == CopyDest;
}

void validateRDGName(std::string_view name, std::string_view kind)
{
	CHECK(Argument, !name.empty(), "An RDG {} must have a name", kind);
}

void validateRDGAccessMode(const RDGPassNode& pass, const RDGResource& resource, const RDGResourceAccess& access)
{
	CHECK(isSingleState(access.state),
	    "RDG pass '{}' uses unsupported combined state {} for resource '{}'",
	    pass.pass->getName(), static_cast<uint16>(access.state), resource.name);

	CHECK(!reads(access.access) || stateCanRead(access.state),
	    "RDG pass '{}' reads resource '{}' from a write-only state",
	    pass.pass->getName(), resource.name);

	CHECK(!writes(access.access) || stateCanWrite(access.state),
	    "RDG pass '{}' writes resource '{}' in a read-only state",
	    pass.pass->getName(), resource.name);
}

void validateRDGTextureAccess(const RDGPassNode& pass, const RDGTexture& texture, const RDGResourceAccess& access)
{
	validateRDGAccessMode(pass, texture, access);

	bool valid{};
	switch (access.state) {
	case Common:
		valid = true;
		break;

	case RenderTarget:
		valid = hasTextureUsage(texture.desc, RHITextureUsage::RenderTarget) &&
		    !isRHIDepthFormat(texture.desc.format);
		break;

	case DepthWrite:
	case DepthRead:
		valid = hasTextureUsage(texture.desc, RHITextureUsage::DepthStencil) &&
		    isRHIDepthFormat(texture.desc.format);
		break;

	case ShaderResource:
		valid = hasTextureUsage(texture.desc, RHITextureUsage::Sampled);
		break;

	case UnorderedAccess:
		valid = hasTextureUsage(texture.desc, RHITextureUsage::Storage) &&
		    !isRHIDepthFormat(texture.desc.format);
		break;

	case CopySource:
		valid = hasTextureUsage(texture.desc, RHITextureUsage::CopySource);
		break;

	case CopyDest:
		valid = hasTextureUsage(texture.desc, RHITextureUsage::CopyDest);
		break;

	default:
		break;
	}

	CHECK(valid,
	    "RDG pass '{}' state {} is incompatible with texture '{}' format/usage",
	    pass.pass->getName(), static_cast<uint16>(access.state), texture.name);
}

void validateRDGBufferAccess(const RDGPassNode& pass, const RDGBuffer& buffer, const RDGResourceAccess& access)
{
	validateRDGAccessMode(pass, buffer, access);

	bool valid{};
	switch (access.state) {
	case Common:
		valid = true;
		break;

	case ConstantBuffer:
		valid = hasBufferUsage(buffer.desc, RHIBufferUsage::ConstantBuffer);
		break;

	case VertexBuffer:
		valid = hasBufferUsage(buffer.desc, RHIBufferUsage::VertexBuffer);
		break;

	case IndexBuffer:
		valid = hasBufferUsage(buffer.desc, RHIBufferUsage::IndexBuffer);
		break;

	case IndirectBuffer:
		valid = hasBufferUsage(buffer.desc, RHIBufferUsage::IndirectArgument);
		break;

	case ShaderResource:
	case UnorderedAccess:
		valid = hasBufferUsage(buffer.desc, RHIBufferUsage::StorageBuffer) ||
		    hasBufferUsage(buffer.desc, RHIBufferUsage::TypedBuffer);
		break;

	case CopySource:
		valid = hasBufferUsage(buffer.desc, RHIBufferUsage::CopySource);
		break;

	case CopyDest:
		valid = hasBufferUsage(buffer.desc, RHIBufferUsage::CopyDest);
		break;

	default:
		break;
	}

	CHECK(valid, "RDG pass '{}' state {} is incompatible with buffer '{}' usage",
	    pass.pass->getName(), static_cast<uint16>(access.state), buffer.name);
}

void validateRDGResourceAccess(const RDGGraph& graph, const RDGPassNode& pass, const RDGResourceAccess& access)
{
	CHECK(access.resource && access.resource->owner == &graph,
	    "RDG pass '{}' uses a resource owned by another graph", pass.pass->getName());

	CHECK(access.state != Unknown, "RDG pass '{}' declares an unknown state", pass.pass->getName());

	if (access.resource->type == RDGResourceType::Texture) {
		CHECK(
		    access.resource->index < graph.textures.size() &&
		        graph.textures[access.resource->index].get() == access.resource,
		    "RDG pass '{}' uses an invalid texture reference", pass.pass->getName());

		validateRDGTextureAccess(pass, *static_cast<const RDGTexture*>(access.resource), access);
		return;
	}

	CHECK(
	    access.resource->index < graph.buffers.size() &&
	        graph.buffers[access.resource->index].get() == access.resource,
	    "RDG pass '{}' uses an invalid buffer reference", pass.pass->getName());

	validateRDGBufferAccess(pass, *static_cast<const RDGBuffer*>(access.resource), access);
}

void validateRDGRasterPass(const RDGPassNode& pass)
{
	CHECK(pass.pass->getDesc().render_targets, "Raster RDG pass requires render target declarations");
	const RDGTexture* reference{};

	auto include = [&](const RDGTexture& texture) {
		if (!reference) {
			reference = &texture;
			return;
		}

		CHECK(
		    texture.desc.width == reference->desc.width &&
		        texture.desc.height == reference->desc.height,
		    "RDG raster pass '{}' has attachments with different extents", pass.pass->getName());

		CHECK(texture.desc.sample_count == reference->desc.sample_count,
		    "RDG raster pass '{}' has attachments with different sample counts", pass.pass->getName());
	};

	for (const auto& attachment : pass.pass->getDesc().render_targets->colors) {
		const auto& texture = *attachment.texture;
		CHECK(!isRHIDepthFormat(texture.desc.format),
		    "RDG raster pass '{}' uses depth texture '{}' as a color attachment",
		    pass.pass->getName(), texture.name);

		include(texture);
	}

	if (pass.pass->getDesc().render_targets->depth) {
		const auto& texture = *pass.pass->getDesc().render_targets->depth->texture;
		CHECK(isRHIDepthFormat(texture.desc.format),
		    "RDG raster pass '{}' uses color texture '{}' as a depth attachment",
		    pass.pass->getName(), texture.name);

		include(texture);
	}
}

void validateRDGCopyPass(const RDGPassNode& pass)
{
	const RDGTexture* source{};
	const RDGTexture* destination{};

	uint32 source_count{};
	uint32 destination_count{};
	for (const auto& access : pass.pass->getDesc().access.accesses) {
		if (access.resource->type != RDGResourceType::Texture)
			continue;

		if (access.state == CopySource && reads(access.access)) {
			source = static_cast<const RDGTexture*>(access.resource);
			++source_count;
		}

		if (access.state == CopyDest && writes(access.access)) {
			destination = static_cast<const RDGTexture*>(access.resource);
			++destination_count;
		}
	}

	if (source_count != 1 || destination_count != 1)
		return;

	const auto& lhs = source->desc;
	const auto& rhs = destination->desc;
	const bool compatible =
	    lhs.format == rhs.format &&
	    lhs.width == rhs.width &&
	    lhs.height == rhs.height &&
	    lhs.depth == rhs.depth &&
	    lhs.array_layers == rhs.array_layers &&
	    lhs.mip_levels == rhs.mip_levels &&
	    lhs.sample_count == rhs.sample_count &&
	    lhs.dimension == rhs.dimension;
	CHECK(compatible,
	    "RDG copy pass '{}' has incompatible textures '{}' and '{}'",
	    pass.pass->getName(), source->name, destination->name);
}

void validateRDGPass(const RDGGraph& graph, RDGPassHandle handle)
{
	CHECK(Argument, handle.valid() && handle.index < graph.passes.size(),
	    "Cannot validate an invalid RDG pass handle");

	const auto& pass = graph.passes[handle.index];
	CHECK(pass.pass, "RDG pass {} has no implementation", handle.index);
	CHECK(!pass.pass->getName().empty(), "RDG pass {} has no name", handle.index);


	CHECK(pass.pass->getType() == RDGPassType::Raster || !pass.pass->getDesc().render_targets.has_value(),
	    "Non-raster RDG pass '{}' declares raster attachments", pass.pass->getName());

	for (const auto& access : pass.pass->getDesc().access.accesses)
		validateRDGResourceAccess(graph, pass, access);

	if (pass.pass->getType() == RDGPassType::Raster)
		validateRDGRasterPass(pass);

	if (pass.pass->getType() == RDGPassType::Copy)
		validateRDGCopyPass(pass);
}

void validateRDGGraph(const RDGGraph& graph)
{
	for (uint32 index = 0; index < graph.passes.size(); ++index)
		validateRDGPass(graph, {index});
}

}        // namespace Vortex

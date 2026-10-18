module Runtime.RHI;

namespace Vortex {

template <typename Flags>
static bool hasFlag(Flags value, Flags flag) noexcept
{
	return (value & flag) != static_cast<Flags>(0);
}

static RHIExtent mipExtent(const RHITextureDesc& desc, uint32 mip_level) noexcept
{
	return {
	    std::max(1U, desc.width >> mip_level),
	    std::max(1U, desc.height >> mip_level),
	};
}

RHITextureSlice normalizeRHITextureSlice(const RHITextureDesc& desc, RHITextureSlice slice)
{
	CHECK(Argument, slice.mip_level < desc.mip_levels && slice.array_layer < desc.array_layers && slice.x >= 0 && slice.y >= 0 && slice.z >= 0, "Invalid texture slice");
	const auto mip = slice.mip_level;
	const uint32 width = std::max(1u, desc.width >> mip);
	const uint32 height = std::max(1u, desc.height >> mip);
	const uint32 depth = std::max(1u, desc.depth >> mip);
	CHECK(Argument, uint32(slice.x) < width && uint32(slice.y) < height && uint32(slice.z) < depth,
	    "Texture slice origin is outside the selected mip");
	if (slice.width < 0)
		slice.width = width - slice.x;
	if (slice.height < 0)
		slice.height = height - slice.y;
	if (slice.depth < 0)
		slice.depth = depth - slice.z;
	CHECK(Argument, slice.width > 0 && slice.height > 0 && slice.depth > 0 && uint32(slice.width) <= width - slice.x && uint32(slice.height) <= height - slice.y && uint32(slice.depth) <= depth - slice.z, "Texture slice extent is outside the selected mip");
	return slice;
}

void validateRHITextureCopy(const RHITextureDesc& dst, const RHITextureSlice& d,
    const RHITextureDesc& src, const RHITextureSlice& s)
{
	CHECK(Argument, dst.format == src.format && dst.sample_count == src.sample_count && d.width == s.width && d.height == s.height && d.depth == s.depth,
	    "Texture copy requires matching formats, sample counts and slice extents");
	CHECK(Argument, !hasRHIStencil(src.format), "Separate depth/stencil plane copies are not exposed by the RHI");
	if (isRHIDepthFormat(src.format) || src.sample_count > 1) {
		auto whole = [](const RHITextureDesc& desc, const RHITextureSlice& slice) {
			return slice.x == 0 && slice.y == 0 && slice.z == 0 &&
			    uint32(slice.width) == std::max(1u, desc.width >> slice.mip_level) &&
			    uint32(slice.height) == std::max(1u, desc.height >> slice.mip_level) &&
			    uint32(slice.depth) == std::max(1u, desc.depth >> slice.mip_level);
		};
		CHECK(Argument, whole(dst, d) && whole(src, s), "Depth/MSAA copies require entire subresources");
	}
}

RHIViewportState normalizeRHIViewportState(const RHIViewportState& state, const RHIFramebufferDesc& framebuffer)
{
	CHECK(Argument, state.viewports.size() <= 1 && state.scissors.size() <= 1, "The RHI currently supports one viewport and scissor");
	auto result = state;
	if (result.viewports.empty())
		result.viewports.emplace_back(float(framebuffer.width), float(framebuffer.height));
	if (result.scissors.empty())
		result.scissors.emplace_back(int(framebuffer.width), int(framebuffer.height));
	const auto& view = result.viewports.front();
	const auto& rect = result.scissors.front();
	CHECK(Argument, view.width() > 0 && view.height() > 0 && view.z_min >= 0 && view.z_max <= 1 && view.z_min <= view.z_max && rect.x_min >= 0 && rect.y_min >= 0 && rect.width() >= 0 && rect.height() >= 0, "Invalid viewport or scissor");
	return result;
}

void validateRHIBufferCopy(const RHIBuffer* dst, uint64 dst_offset, const RHIBuffer* src, uint64 src_offset, uint64 size)
{
	CHECK(Argument, dst && src && dst != src && size && dst_offset <= dst->getDesc().size && size <= dst->getDesc().size - dst_offset && src_offset <= src->getDesc().size && size <= src->getDesc().size - src_offset, "Buffer copy range is invalid");
}

void validateRHITextureState(const RHITexture& texture, RHIResourceState expected, std::string_view operation)
{
	CHECK((texture.getState() & expected) == expected, "Texture '{}' is not in the required state for {}",
	    texture.getName(), operation);
}

void validateRHIBufferState(const RHIBuffer& buffer, RHIResourceState expected, std::string_view operation)
{
	CHECK((buffer.getState() & expected) == expected, "Buffer '{}' is not in the required state for {}",
	    buffer.getName(), operation);
}

void validateRHIBufferTransition(const RHIBuffer& buffer, RHIResourceState before, RHIResourceState after)
{
	CHECK(before == buffer.getState() && after != Unknown, "Invalid buffer transition state");
	if (buffer.getDesc().access == RHIAccessMode::Write) {
		constexpr auto read_states = Common | ConstantBuffer | VertexBuffer | IndexBuffer | IndirectBuffer | ShaderResource | TransferSource;
		CHECK((after & ~read_states) == 0, "Upload buffers must remain in a read-only GPU state");
	} else if (buffer.getDesc().access == RHIAccessMode::Read) {
		CHECK(after == TransferDest, "Readback buffers must remain in TransferDest");
	}
}

void validateRHIBindingSets(const std::vector<RHIRef<RHIBindingSet>>& sets, const std::vector<RHIRef<RHIBindingLayout>>& layouts)
{
	CHECK(Argument, sets.size() == layouts.size(), "Binding set count does not match the pipeline");
	for (size_t i = 0; i < sets.size(); ++i) {
		CHECK(Argument, sets[i] && sets[i]->getLayout() == layouts[i].get(), "Binding set layout mismatch at set {}", i);
		const auto* desc = sets[i]->getDesc();
		if (!desc)
			continue;
		for (const auto& item : desc->bindings) {
			const bool uav = item.type == RHIBindingType::TextureUAV || item.type == RHIBindingType::TypedBufferUAV ||
			    item.type == RHIBindingType::StructuredBufferUAV || item.type == RHIBindingType::RawBufferUAV;
			if (auto* view = dynamic_cast<RHIBufferView*>(item.resource.get()))
				validateRHIBufferState(view->getBuffer(), item.type == RHIBindingType::ConstantBuffer ? ConstantBuffer : uav ? UnorderedAccess :
				                                                                                                               ShaderResource,
				    "bind resource");
			else if (auto* view = dynamic_cast<RHITextureView*>(item.resource.get()))
				validateRHITextureState(view->getTexture(), uav ? UnorderedAccess : ShaderResource, "bind resource");
		}
	}
}

void validateRHICommandListDesc(const RHICommandListDesc& desc)
{
	CHECK(Argument, desc.queue_type == RHICommandQueue::Graphics || desc.queue_type == RHICommandQueue::Compute || desc.queue_type == RHICommandQueue::Copy, "Invalid command recording queue");
}

void validateRHIBufferDesc(const RHIBufferDesc& desc)
{
	if (desc.size == 0)
		ERROR(Argument, "Invalid RHI descriptor: buffer size must be non-zero");

	if (desc.usage == RHIBufferUsage::None)
		ERROR(Argument, "Invalid RHI descriptor: buffer usage must be non-empty");

	if (desc.stride > desc.size)
		ERROR(Argument, "Invalid RHI descriptor: buffer stride cannot exceed its size");

	if (desc.access != RHIAccessMode::None && desc.access != RHIAccessMode::Read && desc.access != RHIAccessMode::Write)
		ERROR(Argument, "Invalid RHI descriptor: buffer access mode is unknown");
}

RHIBufferViewDesc normalizeRHIBufferViewDesc(const RHIBufferViewDesc& desc)
{
	if (!desc.buffer)
		ERROR(Argument, "Invalid RHI descriptor: buffer view requires a buffer");

	const auto& buffer = desc.buffer->getDesc();
	validateRHIBufferDesc(buffer);
	if (desc.offset >= buffer.size)
		ERROR(Argument, "Invalid RHI descriptor: buffer view offset is outside the buffer");

	auto normalized = desc;
	normalized.size = desc.size == 0 ? buffer.size - desc.offset : desc.size;
	if (normalized.size == 0 || normalized.size > buffer.size - desc.offset)
		ERROR(Argument, "Invalid RHI descriptor: buffer view range is outside the buffer");

	switch (desc.type) {
	case RHIBufferViewType::Constant:
		if (!hasFlag(buffer.usage, RHIBufferUsage::ConstantBuffer))
			ERROR(Argument, "Invalid RHI descriptor: constant view requires ConstantBuffer usage");
		break;

	case RHIBufferViewType::Structured:
		normalized.stride = desc.stride == 0 ? buffer.stride : desc.stride;
		if (!hasFlag(buffer.usage, RHIBufferUsage::StorageBuffer) || normalized.stride == 0)
			ERROR(Argument, "Invalid RHI descriptor: structured view requires StorageBuffer usage and a non-zero stride");
		if (normalized.offset % normalized.stride != 0 || normalized.size % normalized.stride != 0)
			ERROR(Argument, "Invalid RHI descriptor: structured view range must be aligned to its stride");
		break;

	case RHIBufferViewType::Typed:
	{
		const auto format_size = getRHIFormatByteSize(desc.format);
		if (!hasFlag(buffer.usage, RHIBufferUsage::TypedBuffer) || format_size == 0 || isRHIDepthFormat(desc.format))
			ERROR(Argument, "Invalid RHI descriptor: typed view requires TypedBuffer usage and a concrete color format");
		if (normalized.offset % format_size != 0 || normalized.size % format_size != 0)
			ERROR(Argument, "Invalid RHI descriptor: typed view range must be aligned to its format size");
		break;
	}

	case RHIBufferViewType::Raw:
		if (!hasFlag(buffer.usage, RHIBufferUsage::StorageBuffer) || normalized.offset % 4 != 0 || normalized.size % 4 != 0)
			ERROR(Argument, "Invalid RHI descriptor: raw view requires StorageBuffer usage and a 4-byte aligned range");
		break;

	default:
		ERROR(Argument, "Invalid RHI descriptor: buffer view type is unknown");
	}

	return normalized;
}

void validateRHITextureDesc(const RHITextureDesc& desc)
{
	if (desc.width == 0 || desc.height == 0 || desc.depth == 0 || desc.array_layers == 0 || desc.mip_levels == 0)
		ERROR(Argument, "Invalid RHI descriptor: texture extent, layers, and mip count must be non-zero");

	if (getRHIFormatByteSize(desc.format) == 0)
		ERROR(Argument, "Invalid RHI descriptor: texture format must be concrete");

	if (desc.usage == RHITextureUsage::None)
		ERROR(Argument, "Invalid RHI descriptor: texture usage must be non-empty");

	if (!isRHISampleCountValid(desc.sample_count))
		ERROR(Argument, "Invalid RHI descriptor: texture sample count is unsupported");

	const bool depth_format = isRHIDepthFormat(desc.format);
	if (hasFlag(desc.usage, RHITextureUsage::DepthStencil) != depth_format)
		ERROR(Argument, "Invalid RHI descriptor: DepthStencil usage and texture format disagree");

	if (depth_format && hasFlag(desc.usage, RHITextureUsage::RenderTarget))
		ERROR(Argument, "Invalid RHI descriptor: depth formats cannot use RenderTarget usage");

	if (desc.sample_count > 1 && (desc.mip_levels > 1 || hasFlag(desc.usage, RHITextureUsage::Storage)))
		ERROR(Argument, "Invalid RHI descriptor: multisampled textures cannot have mip chains or Storage usage");

	switch (desc.dimension) {
	case RHITextureDimension::Texture1D:
		if (desc.height != 1 || desc.depth != 1)
			ERROR(Argument, "Invalid RHI descriptor: 1D textures require height and depth equal to one");
		break;

	case RHITextureDimension::Texture2D:
		if (desc.depth != 1)
			ERROR(Argument, "Invalid RHI descriptor: 2D textures require depth equal to one");
		break;

	case RHITextureDimension::Texture3D:
		if (desc.array_layers != 1)
			ERROR(Argument, "Invalid RHI descriptor: 3D textures cannot have array layers");
		break;

	case RHITextureDimension::TextureCube:
		if (desc.depth != 1 || desc.width != desc.height || desc.array_layers % 6 != 0)
			ERROR(Argument, "Invalid RHI descriptor: cube textures require square faces and six-layer groups");
		break;

	default:
		ERROR(Argument, "Invalid RHI descriptor: texture dimension is unknown");
	}

	const uint32 largest_dimension = std::max({desc.width, desc.height, desc.depth});
	if (desc.mip_levels > std::bit_width(largest_dimension))
		ERROR(Argument, "Invalid RHI descriptor: texture mip count exceeds its extent");
}

RHITextureViewDesc normalizeRHITextureViewDesc(const RHITextureViewDesc& desc)
{
	if (!desc.texture)
		ERROR(Argument, "Invalid RHI descriptor: texture view requires a texture");

	const auto& texture = desc.texture->getDesc();
	validateRHITextureDesc(texture);

	auto normalized = desc;
	normalized.format = desc.format == RHIFormat::Unknown ? texture.format : desc.format;
	if (normalized.format != texture.format)
		ERROR(Argument, "Invalid RHI descriptor: texture format reinterpretation is unsupported");

	if (normalized.dimension == RHITextureViewDimension::Automatic) {
		switch (texture.dimension) {
		case RHITextureDimension::Texture1D:
			normalized.dimension = texture.array_layers > 1 ? RHITextureViewDimension::Texture1DArray : RHITextureViewDimension::Texture1D;
			break;

		case RHITextureDimension::Texture2D:
			normalized.dimension = texture.array_layers > 1 ? RHITextureViewDimension::Texture2DArray : RHITextureViewDimension::Texture2D;
			break;

		case RHITextureDimension::Texture3D:
			normalized.dimension = RHITextureViewDimension::Texture3D;
			break;

		case RHITextureDimension::TextureCube:
			normalized.dimension = texture.array_layers > 6 ? RHITextureViewDimension::TextureCubeArray : RHITextureViewDimension::TextureCube;
			break;
		}
	}

	bool dimension_compatible{};
	switch (texture.dimension) {
	case RHITextureDimension::Texture1D:
		dimension_compatible = normalized.dimension == RHITextureViewDimension::Texture1D || normalized.dimension == RHITextureViewDimension::Texture1DArray;
		break;

	case RHITextureDimension::Texture2D:
		dimension_compatible = normalized.dimension == RHITextureViewDimension::Texture2D || normalized.dimension == RHITextureViewDimension::Texture2DArray;
		break;

	case RHITextureDimension::Texture3D:
		dimension_compatible = normalized.dimension == RHITextureViewDimension::Texture3D;
		break;

	case RHITextureDimension::TextureCube:
		dimension_compatible = normalized.dimension == RHITextureViewDimension::Texture2D ||
		    normalized.dimension == RHITextureViewDimension::Texture2DArray ||
		    normalized.dimension == RHITextureViewDimension::TextureCube ||
		    normalized.dimension == RHITextureViewDimension::TextureCubeArray;
		break;
	}

	if (!dimension_compatible)
		ERROR(Argument, "Invalid RHI descriptor: texture view dimension is incompatible with its texture");

	auto& subresource = normalized.subresource;
	if (subresource.base_mip_level >= texture.mip_levels || subresource.base_array_layer >= texture.array_layers)
		ERROR(Argument, "Invalid RHI descriptor: texture view subresource begins outside the texture");

	subresource.level_count = subresource.level_count == 0 ? texture.mip_levels - subresource.base_mip_level : subresource.level_count;
	subresource.layer_count = subresource.layer_count == 0 ? texture.array_layers - subresource.base_array_layer : subresource.layer_count;
	if (subresource.level_count == 0 || subresource.layer_count == 0 ||
	    subresource.level_count > texture.mip_levels - subresource.base_mip_level ||
	    subresource.layer_count > texture.array_layers - subresource.base_array_layer)
		ERROR(Argument, "Invalid RHI descriptor: texture view subresource range is outside the texture");

	if ((normalized.dimension == RHITextureViewDimension::Texture1D ||
	        normalized.dimension == RHITextureViewDimension::Texture2D ||
	        normalized.dimension == RHITextureViewDimension::Texture3D) &&
	    subresource.layer_count != 1)
		ERROR(Argument, "Invalid RHI descriptor: non-array texture views must select one layer");

	if (normalized.dimension == RHITextureViewDimension::TextureCube && subresource.layer_count != 6)
		ERROR(Argument, "Invalid RHI descriptor: cube texture views must select six layers");

	if ((normalized.dimension == RHITextureViewDimension::TextureCube || normalized.dimension == RHITextureViewDimension::TextureCubeArray) &&
	    (subresource.base_array_layer % 6 != 0 || subresource.layer_count % 6 != 0))
		ERROR(Argument, "Invalid RHI descriptor: cube texture views require six-layer aligned subresources");

	switch (normalized.type) {
	case RHITextureViewType::ShaderResource:
		if (!hasFlag(texture.usage, RHITextureUsage::Sampled))
			ERROR(Argument, "Invalid RHI descriptor: shader resource view requires Sampled usage");
		break;

	case RHITextureViewType::UnorderedAccess:
		if (!hasFlag(texture.usage, RHITextureUsage::Storage) || isRHIDepthFormat(normalized.format))
			ERROR(Argument, "Invalid RHI descriptor: unordered access view requires color Storage usage");
		break;

	case RHITextureViewType::RenderTarget:
		if (!hasFlag(texture.usage, RHITextureUsage::RenderTarget) || isRHIDepthFormat(normalized.format))
			ERROR(Argument, "Invalid RHI descriptor: render target view requires color RenderTarget usage");
		break;

	case RHITextureViewType::DepthStencil:
		if (!hasFlag(texture.usage, RHITextureUsage::DepthStencil) || !isRHIDepthFormat(normalized.format))
			ERROR(Argument, "Invalid RHI descriptor: depth stencil view requires a depth format and DepthStencil usage");
		break;

	default:
		ERROR(Argument, "Invalid RHI descriptor: texture view type is unknown");
	}

	return normalized;
}

void validateRHISamplerDesc(const RHISamplerDesc& desc)
{
	if (!std::isfinite(desc.mip_bias))
		ERROR(Argument, "Invalid RHI descriptor: sampler mip bias must be finite");
}

void validateRHIShaderDesc(const RHIShaderDesc& desc, std::span<const std::byte> bytecode)
{
	if (desc.type != RHIShaderType::Vertex && desc.type != RHIShaderType::Pixel &&
	    desc.type != RHIShaderType::Geometry && desc.type != RHIShaderType::Compute)
		ERROR(Argument, "Invalid RHI descriptor: shader type must contain exactly one stage");

	if (desc.entry_point.empty())
		ERROR(Argument, "Invalid RHI descriptor: shader entry point cannot be empty");

	if (bytecode.empty() || bytecode.size() % sizeof(uint32) != 0)
		ERROR(Argument, "Invalid RHI descriptor: shader bytecode must be non-empty and 4-byte sized");
}

void validateRHIFramebufferInfo(const RHIFramebufferInfo& info)
{
	CHECK(Argument, info.color_formats.size() <= 8, "At most eight color attachments are supported");
	if (!isRHISampleCountValid(info.sample_count))
		ERROR(Argument, "Invalid RHI descriptor: framebuffer sample count is unsupported");

	if (info.color_formats.empty() && info.depth_format == RHIFormat::Unknown)
		ERROR(Argument, "Invalid RHI descriptor: framebuffer info requires a color or depth format");

	for (const auto format : info.color_formats)
		if (getRHIFormatByteSize(format) == 0 || isRHIDepthFormat(format))
			ERROR(Argument, "Invalid RHI descriptor: framebuffer color formats must be concrete color formats");

	if (info.depth_format != RHIFormat::Unknown && !isRHIDepthFormat(info.depth_format))
		ERROR(Argument, "Invalid RHI descriptor: framebuffer depth format must be a depth/stencil format");
}

void validateRHIFramebufferDesc(const RHIFramebufferDesc& desc)
{
	CHECK(Argument, desc.color_attachments.size() <= 8, "At most eight color attachments are supported");
	if (desc.width == 0 || desc.height == 0 || desc.array_size == 0)
		ERROR(Argument, "Invalid RHI descriptor: framebuffer extent and array size must be non-zero");

	if (desc.color_attachments.empty() && !desc.depth_attachment.texture_view)
		ERROR(Argument, "Invalid RHI descriptor: framebuffer requires at least one attachment");

	auto validateAttachment = [&](const RHIFramebufferAttachment& attachment, bool depth) {
		if (!attachment.texture_view)
			ERROR(Argument, "Invalid RHI descriptor: framebuffer attachment cannot be empty");

		const auto& view = attachment.texture_view->getDesc();
		const auto& texture = attachment.texture_view->getTexture().getDesc();
		if (view.type != (depth ? RHITextureViewType::DepthStencil : RHITextureViewType::RenderTarget))
			ERROR(Argument, "Invalid RHI descriptor: framebuffer attachment view type is incompatible");

		CHECK(Argument, attachment.load_op == RHILoadOp::Load || attachment.load_op == RHILoadOp::Clear || attachment.load_op == RHILoadOp::Discard || attachment.load_op == RHILoadOp::None,
		    "Invalid attachment load operation");
		CHECK(Argument, attachment.store_op == RHIStoreOp::Store || attachment.store_op == RHIStoreOp::Discard || attachment.store_op == RHIStoreOp::None,
		    "Invalid attachment store operation");
		if (attachment.read_only && (!depth || attachment.load_op != RHILoadOp::Load || (attachment.store_op != RHIStoreOp::Store && attachment.store_op != RHIStoreOp::None)))
			ERROR(Argument, "Invalid RHI descriptor: read-only depth requires Load and Store/None");

		if (isRHIDepthFormat(view.format) != depth)
			ERROR(Argument, "Invalid RHI descriptor: framebuffer attachment format is incompatible");

		if (view.subresource.level_count != 1)
			ERROR(Argument, "Invalid RHI descriptor: framebuffer attachments must select one mip level");

		if (desc.array_size > view.subresource.layer_count)
			ERROR(Argument, "Invalid RHI descriptor: framebuffer array size exceeds its attachment view layers");

		const auto extent = mipExtent(texture, view.subresource.base_mip_level);
		if (extent.width != desc.width || extent.height != desc.height || texture.sample_count != desc.sample_count)
			ERROR(Argument, "Invalid RHI descriptor: framebuffer attachment extent or sample count does not match");
	};

	for (const auto& attachment : desc.color_attachments)
		validateAttachment(attachment, false);

	if (desc.depth_attachment.texture_view)
		validateAttachment(desc.depth_attachment, true);

	validateRHIFramebufferInfo(RHIFramebufferInfo(desc));
}

void validateRHIInputLayoutDesc(const RHIInputLayoutDesc& desc)
{
	std::unordered_map<uint32, uint32> strides;
	for (const auto& binding : desc.binding_descs)
		if (binding.stride == 0 || !strides.emplace(binding.binding, binding.stride).second)
			ERROR(Argument, "Invalid RHI descriptor: input bindings require unique slots and non-zero strides");

	std::unordered_set<uint32> locations;
	for (const auto& attribute : desc.attribute_descs) {
		const auto binding = strides.find(attribute.binding);
		const auto format_size = getRHIFormatByteSize(attribute.format);
		if (binding == strides.end())
			ERROR(Argument, "Invalid RHI descriptor: vertex attribute references a missing binding");

		if (format_size == 0 || isRHIDepthFormat(attribute.format))
			ERROR(Argument, "Invalid RHI descriptor: vertex attribute requires a concrete color format");

		if (attribute.offset > binding->second || format_size > binding->second - attribute.offset)
			ERROR(Argument, "Invalid RHI descriptor: vertex attribute exceeds its binding stride");

		if (!locations.insert(attribute.location).second)
			ERROR(Argument, "Invalid RHI descriptor: vertex attribute locations must be unique");
	}
}

void validateRHIBindingLayoutDesc(const RHIBindingLayoutDesc& desc)
{
	if (desc.visibility == RHIShaderType::None)
		ERROR(Argument, "Invalid RHI descriptor: binding layout requires shader visibility");

	std::unordered_set<uint32> slots;
	uint64 descriptor_count{};
	auto register_type = [](RHIBindingType type) {
		if (type == RHIBindingType::Sampler)
			return 0;
		if (type == RHIBindingType::ConstantBuffer)
			return 1;
		if (type == RHIBindingType::TextureUAV || type == RHIBindingType::TypedBufferUAV ||
		    type == RHIBindingType::StructuredBufferUAV || type == RHIBindingType::RawBufferUAV)
			return 2;
		return 3;
	};
	for (bool push_constants{}; const auto& item : desc.bindings) {
		if (item.type == RHIBindingType::None || item.type > RHIBindingType::Sampler)
			ERROR(Argument, "Invalid RHI descriptor: binding layout item requires a concrete type");

		if (item.type == RHIBindingType::PushConstants) {
			if (push_constants || item.size == 0 || item.size > 128 || item.size % 4 != 0)
				ERROR(Argument, "Invalid RHI descriptor: one 4-byte aligned push constant range up to 128 bytes is supported");

			push_constants = true;
			continue;
		}

		if (!slots.insert(item.slot).second)
			ERROR(Argument, "Invalid RHI descriptor: descriptor binding slots must be unique");
		CHECK(item.array_size && uint64(item.slot) + item.array_size <= uint64(~uint32(0)) + 1,
		    "Descriptor arrays require a nonzero size and a representable register range");
		descriptor_count += item.array_size;
		CHECK(descriptor_count <= ~uint32(0), "Binding layout descriptor count exceeds UINT32_MAX");
		for (const auto& other : desc.bindings) {
			if (&other == &item)
				break;
			if (other.type == RHIBindingType::PushConstants || register_type(other.type) != register_type(item.type))
				continue;
			CHECK(uint64(item.slot) >= uint64(other.slot) + other.array_size || uint64(other.slot) >= uint64(item.slot) + item.array_size,
			    "Descriptor arrays overlap in the D3D register namespace");
		}
	}
}

void validateRHIBindlessLayoutDesc(const RHIBindlessLayoutDesc& desc)
{
	CHECK(desc.visibility != RHIShaderType::None, "Bindless layouts require shader visibility");
	CHECK(desc.type != RHIBindingType::None && desc.type != RHIBindingType::PushConstants && desc.type <= RHIBindingType::Sampler,
	    "Bindless layouts require a concrete descriptor type");
	CHECK(desc.max_capacity && uint64(desc.slot) + desc.max_capacity <= uint64(~uint32(0)) + 1,
	    "Bindless layouts require a nonzero capacity and a representable register range");
}

static void validateBindingResource(const RHIBindingSetItem& binding)
{
	if (!binding.resource)
		ERROR(Argument, "Invalid RHI descriptor: binding set resource cannot be empty");

	switch (binding.type) {
	case RHIBindingType::TextureSRV:
	case RHIBindingType::TextureUAV:
	{
		auto* view = dynamic_cast<RHITextureView*>(binding.resource.get());
		const auto expected = binding.type == RHIBindingType::TextureSRV ? RHITextureViewType::ShaderResource : RHITextureViewType::UnorderedAccess;
		if (!view || view->getDesc().type != expected)
			ERROR(Argument, "Invalid RHI descriptor: texture binding requires a compatible texture view");
		break;
	}

	case RHIBindingType::ConstantBuffer:
	case RHIBindingType::TypedBufferSRV:
	case RHIBindingType::TypedBufferUAV:
	case RHIBindingType::StructuredBufferSRV:
	case RHIBindingType::StructuredBufferUAV:
	case RHIBindingType::RawBufferSRV:
	case RHIBindingType::RawBufferUAV:
	{
		auto* view = dynamic_cast<RHIBufferView*>(binding.resource.get());
		if (!view)
			ERROR(Argument, "Invalid RHI descriptor: buffer binding requires a buffer view");

		RHIBufferViewType expected = RHIBufferViewType::Raw;
		if (binding.type == RHIBindingType::ConstantBuffer)
			expected = RHIBufferViewType::Constant;
		else if (binding.type == RHIBindingType::TypedBufferSRV || binding.type == RHIBindingType::TypedBufferUAV)
			expected = RHIBufferViewType::Typed;
		else if (binding.type == RHIBindingType::StructuredBufferSRV || binding.type == RHIBindingType::StructuredBufferUAV)
			expected = RHIBufferViewType::Structured;

		if (view->getDesc().type != expected)
			ERROR(Argument, "Invalid RHI descriptor: buffer binding view type is incompatible");
		break;
	}

	case RHIBindingType::Sampler:
		if (!dynamic_cast<RHISampler*>(binding.resource.get()))
			ERROR(Argument, "Invalid RHI descriptor: sampler binding requires a sampler resource");
		break;

	default:
		ERROR(Argument, "Invalid RHI descriptor: binding set type is unsupported");
	}
}

void validateRHIBindingSetDesc(const RHIBindingSetDesc& desc, const RHIBindingLayout& layout)
{
	const auto* layout_desc = layout.getDesc();
	CHECK(layout_desc, "Use createDescriptorTable for a bindless layout");
	validateRHIBindingLayoutDesc(*layout_desc);
	uint64 expected_count{};
	for (const auto& item : layout_desc->bindings)
		if (item.type != RHIBindingType::PushConstants)
			expected_count += item.array_size;

	if (desc.bindings.size() != expected_count)
		ERROR(Argument, "Invalid RHI descriptor: binding set must provide every non-push-constant layout item");

	std::unordered_set<uint64> slots;
	for (const auto& binding : desc.bindings) {
		if (binding.type == RHIBindingType::PushConstants || binding.type == RHIBindingType::None)
			ERROR(Argument, "Invalid RHI descriptor: binding sets cannot contain push constants or empty items");

		if (!slots.insert((uint64(binding.slot) << 32) | binding.array_element).second)
			ERROR(Argument, "Invalid RHI descriptor: binding slot and array element must be unique");

		const auto layout_item = std::ranges::find_if(layout_desc->bindings, [&](const auto& item) {
			return item.slot == binding.slot && item.type == binding.type;
		});

		if (layout_item == layout_desc->bindings.end())
			ERROR(Argument, "Invalid RHI descriptor: binding set item does not match its layout slot and type");
		CHECK(binding.array_element < layout_item->array_size, "Descriptor array element exceeds the layout array size");

		validateBindingResource(binding);
	}
}

void validateRHIBindingSetItem(const RHIBindingSetItem& item, const RHIBindlessLayoutDesc& layout)
{
	validateRHIBindlessLayoutDesc(layout);
	CHECK(item.type == layout.type && item.slot == layout.slot, "Descriptor table item does not match its layout slot and type");
	CHECK(item.array_element < layout.max_capacity, "Descriptor table element exceeds the layout maximum capacity");
	validateBindingResource(item);
}

void validateRHIGraphicsPipelineDesc(const RHIGraphicsPipelineDesc& desc)
{
	validateRHIFramebufferInfo(desc.framebuffer_info);
	if (!desc.vertex_shader || desc.vertex_shader->getDesc().type != RHIShaderType::Vertex)
		ERROR(Argument, "Invalid RHI descriptor: graphics pipeline requires a vertex shader");

	if (!desc.pixel_shader || desc.pixel_shader->getDesc().type != RHIShaderType::Pixel)
		ERROR(Argument, "Invalid RHI descriptor: graphics pipeline requires a pixel shader");

	if ((desc.depth_state.depth_test_enable || desc.depth_state.depth_write_enable) &&
	    desc.framebuffer_info.depth_format == RHIFormat::Unknown)
		ERROR(Argument, "Invalid RHI descriptor: depth testing or writing requires a depth target format");

	if (desc.depth_state.stencil_test_enable && !hasRHIStencil(desc.framebuffer_info.depth_format))
		ERROR(Argument, "Invalid RHI descriptor: stencil testing requires a stencil-capable depth target format");

	if (desc.blend_state.blend_descs.size() > desc.framebuffer_info.color_formats.size())
		ERROR(Argument, "Invalid RHI descriptor: blend attachment count exceeds render target color count");

	if (desc.blend_state.alpha_to_coverage_enable && desc.framebuffer_info.sample_count == 1)
		ERROR(Argument, "Invalid RHI descriptor: alpha-to-coverage requires multisampling");

	std::unordered_set<const RHIBindingLayout*> layouts;

	for (bool push_constants{}; const auto& layout : desc.binding_layouts) {
		if (!layout || !layouts.insert(layout.get()).second)
			ERROR(Argument, "Invalid RHI descriptor: graphics pipeline binding layouts must be non-null and unique");

		if (const auto* bindless = layout->getBindlessDesc()) {
			validateRHIBindlessLayoutDesc(*bindless);
			continue;
		}
		const auto* layout_desc = layout->getDesc();
		CHECK(layout_desc, "Binding layout has no description");
		validateRHIBindingLayoutDesc(*layout_desc);
		for (const auto& item : layout_desc->bindings) {
			if (item.type != RHIBindingType::PushConstants)
				continue;

			if (push_constants)
				ERROR(Argument, "Invalid RHI descriptor: graphics pipeline supports one push constant range");

			push_constants = true;
		}
	}
}

void validateRHIComputePipelineDesc(const RHIComputePipelineDesc& desc)
{
	if (!desc.compute_shader || desc.compute_shader->getDesc().type != RHIShaderType::Compute)
		ERROR(Argument, "Invalid RHI descriptor: compute pipeline requires a compute shader");

	std::unordered_set<const RHIBindingLayout*> layouts;
	for (bool push_constants{}; const auto& layout : desc.binding_layouts) {
		if (!layout || !layouts.insert(layout.get()).second)
			ERROR(Argument, "Invalid RHI descriptor: compute pipeline binding layouts must be non-null and unique");

		if (const auto* bindless = layout->getBindlessDesc()) {
			validateRHIBindlessLayoutDesc(*bindless);
			CHECK((bindless->visibility & RHIShaderType::Compute) != RHIShaderType::None,
			    "Invalid RHI descriptor: compute pipeline binding layouts must be visible to compute shaders");
			continue;
		}
		const auto* layout_desc = layout->getDesc();
		CHECK(layout_desc, "Binding layout has no description");
		validateRHIBindingLayoutDesc(*layout_desc);
		if ((layout_desc->visibility & RHIShaderType::Compute) == RHIShaderType::None)
			ERROR(Argument, "Invalid RHI descriptor: compute pipeline binding layouts must be visible to compute shaders");

		for (const auto& item : layout_desc->bindings) {
			if (item.type != RHIBindingType::PushConstants)
				continue;

			if (push_constants)
				ERROR(Argument, "Invalid RHI descriptor: compute pipeline supports one push constant range");

			push_constants = true;
		}
	}
}

}        // namespace Vortex

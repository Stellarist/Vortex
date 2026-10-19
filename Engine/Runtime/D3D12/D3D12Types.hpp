export module Runtime.D3D12:Types;

import d3d12;
import Core;
import Runtime.RHI;

export namespace Vortex {

template <typename T>
using D3D12Ptr = Microsoft::WRL::ComPtr<T>;

inline constexpr uint32 D3D12PushConstantSpace = 255;
inline constexpr uint32 D3D12InvalidRootParameter = ~0u;

inline constexpr DXGI_FORMAT toDXGIFormat(RHIFormat format)
{
	switch (format) {
	case RHIFormat::Unknown:
		return DXGI_FORMAT_UNKNOWN;
	case RHIFormat::R8_UINT:
		return DXGI_FORMAT_R8_UINT;
	case RHIFormat::R8_SINT:
		return DXGI_FORMAT_R8_SINT;
	case RHIFormat::R8_UNORM:
		return DXGI_FORMAT_R8_UNORM;
	case RHIFormat::R8_SNORM:
		return DXGI_FORMAT_R8_SNORM;
	case RHIFormat::RG8_UINT:
		return DXGI_FORMAT_R8G8_UINT;
	case RHIFormat::RG8_SINT:
		return DXGI_FORMAT_R8G8_SINT;
	case RHIFormat::RG8_UNORM:
		return DXGI_FORMAT_R8G8_UNORM;
	case RHIFormat::RG8_SNORM:
		return DXGI_FORMAT_R8G8_SNORM;
	case RHIFormat::R16_UINT:
		return DXGI_FORMAT_R16_UINT;
	case RHIFormat::R16_SINT:
		return DXGI_FORMAT_R16_SINT;
	case RHIFormat::R16_UNORM:
		return DXGI_FORMAT_R16_UNORM;
	case RHIFormat::R16_SNORM:
		return DXGI_FORMAT_R16_SNORM;
	case RHIFormat::R16_FLOAT:
		return DXGI_FORMAT_R16_FLOAT;
	case RHIFormat::RGBA8_UINT:
		return DXGI_FORMAT_R8G8B8A8_UINT;
	case RHIFormat::RGBA8_SINT:
		return DXGI_FORMAT_R8G8B8A8_SINT;
	case RHIFormat::RGBA8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case RHIFormat::RGBA8_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case RHIFormat::RGBA8_SNORM:
		return DXGI_FORMAT_R8G8B8A8_SNORM;
	case RHIFormat::BGRA8_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	case RHIFormat::BGRA8_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	case RHIFormat::RG16_UINT:
		return DXGI_FORMAT_R16G16_UINT;
	case RHIFormat::RG16_SINT:
		return DXGI_FORMAT_R16G16_SINT;
	case RHIFormat::RG16_UNORM:
		return DXGI_FORMAT_R16G16_UNORM;
	case RHIFormat::RG16_SNORM:
		return DXGI_FORMAT_R16G16_SNORM;
	case RHIFormat::RG16_FLOAT:
		return DXGI_FORMAT_R16G16_FLOAT;
	case RHIFormat::R32_UINT:
		return DXGI_FORMAT_R32_UINT;
	case RHIFormat::R32_SINT:
		return DXGI_FORMAT_R32_SINT;
	case RHIFormat::R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case RHIFormat::RGBA16_UINT:
		return DXGI_FORMAT_R16G16B16A16_UINT;
	case RHIFormat::RGBA16_SINT:
		return DXGI_FORMAT_R16G16B16A16_SINT;
	case RHIFormat::RGBA16_FLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case RHIFormat::RGBA16_UNORM:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
	case RHIFormat::RGBA16_SNORM:
		return DXGI_FORMAT_R16G16B16A16_SNORM;
	case RHIFormat::RG32_UINT:
		return DXGI_FORMAT_R32G32_UINT;
	case RHIFormat::RG32_SINT:
		return DXGI_FORMAT_R32G32_SINT;
	case RHIFormat::RG32_FLOAT:
		return DXGI_FORMAT_R32G32_FLOAT;
	case RHIFormat::RGB32_UINT:
		return DXGI_FORMAT_R32G32B32_UINT;
	case RHIFormat::RGB32_SINT:
		return DXGI_FORMAT_R32G32B32_SINT;
	case RHIFormat::RGB32_FLOAT:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case RHIFormat::RGBA32_UINT:
		return DXGI_FORMAT_R32G32B32A32_UINT;
	case RHIFormat::RGBA32_SINT:
		return DXGI_FORMAT_R32G32B32A32_SINT;
	case RHIFormat::RGBA32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case RHIFormat::D16_UNORM:
		return DXGI_FORMAT_D16_UNORM;
	case RHIFormat::D24_UNORM_S8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case RHIFormat::D32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;
	default:
		ERROR(Argument, "Unsupported D3D12 format {}", static_cast<uint32>(format));
	}
}

inline constexpr DXGI_FORMAT toD3D12ResourceFormat(RHIFormat format)
{
	switch (format) {
	case RHIFormat::D16_UNORM:
		return DXGI_FORMAT_R16_TYPELESS;
	case RHIFormat::D24_UNORM_S8_UINT:
		return DXGI_FORMAT_R24G8_TYPELESS;
	case RHIFormat::D32_FLOAT:
		return DXGI_FORMAT_R32_TYPELESS;
	case RHIFormat::RGBA8_SRGB:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;
	case RHIFormat::BGRA8_SRGB:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;
	default:
		return toDXGIFormat(format);
	}
}

inline constexpr DXGI_FORMAT toD3D12SRVFormat(RHIFormat format)
{
	switch (format) {
	case RHIFormat::D16_UNORM:
		return DXGI_FORMAT_R16_UNORM;
	case RHIFormat::D24_UNORM_S8_UINT:
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case RHIFormat::D32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	default:
		return toDXGIFormat(format);
	}
}

inline constexpr D3D12_COMPARISON_FUNC toD3D12ComparisonFunc(RHICompareOp op)
{
	switch (op) {
	case RHICompareOp::Never:
		return D3D12_COMPARISON_FUNC_NEVER;
	case RHICompareOp::Less:
		return D3D12_COMPARISON_FUNC_LESS;
	case RHICompareOp::LessOrEqual:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case RHICompareOp::Greater:
		return D3D12_COMPARISON_FUNC_GREATER;
	case RHICompareOp::NotEqual:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case RHICompareOp::GreaterOrEqual:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case RHICompareOp::Always:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	}
	ERROR(Argument, "Invalid depth compare operation");
}

inline constexpr D3D12_BLEND toD3D12Blend(RHIBlendFactor factor)
{
	switch (factor) {
	case RHIBlendFactor::Zero:
		return D3D12_BLEND_ZERO;
	case RHIBlendFactor::One:
		return D3D12_BLEND_ONE;
	case RHIBlendFactor::SrcColor:
		return D3D12_BLEND_SRC_COLOR;
	case RHIBlendFactor::OneMinusSrcColor:
		return D3D12_BLEND_INV_SRC_COLOR;
	case RHIBlendFactor::DstColor:
		return D3D12_BLEND_DEST_COLOR;
	case RHIBlendFactor::OneMinusDstColor:
		return D3D12_BLEND_INV_DEST_COLOR;
	case RHIBlendFactor::SrcAlpha:
		return D3D12_BLEND_SRC_ALPHA;
	case RHIBlendFactor::OneMinusSrcAlpha:
		return D3D12_BLEND_INV_SRC_ALPHA;
	case RHIBlendFactor::DstAlpha:
		return D3D12_BLEND_DEST_ALPHA;
	case RHIBlendFactor::OneMinusDstAlpha:
		return D3D12_BLEND_INV_DEST_ALPHA;
	}
	ERROR(Argument, "Invalid blend factor");
}

inline constexpr D3D12_BLEND_OP toD3D12BlendOp(RHIBlendOp op) noexcept
{
	switch (op) {
	case RHIBlendOp::Add:
		return D3D12_BLEND_OP_ADD;
	case RHIBlendOp::Subtract:
		return D3D12_BLEND_OP_SUBTRACT;
	case RHIBlendOp::ReverseSubtract:
		return D3D12_BLEND_OP_REV_SUBTRACT;
	case RHIBlendOp::Min:
		return D3D12_BLEND_OP_MIN;
	case RHIBlendOp::Max:
		return D3D12_BLEND_OP_MAX;
	}

	return D3D12_BLEND_OP_ADD;
}

inline constexpr UINT8 toD3D12ColorWriteMask(RHIColorMask mask) noexcept
{
	UINT8 flags{};
	if ((mask & RHIColorMask::R) != RHIColorMask::None)
		flags |= D3D12_COLOR_WRITE_ENABLE_RED;
	if ((mask & RHIColorMask::G) != RHIColorMask::None)
		flags |= D3D12_COLOR_WRITE_ENABLE_GREEN;
	if ((mask & RHIColorMask::B) != RHIColorMask::None)
		flags |= D3D12_COLOR_WRITE_ENABLE_BLUE;
	if ((mask & RHIColorMask::A) != RHIColorMask::None)
		flags |= D3D12_COLOR_WRITE_ENABLE_ALPHA;

	return flags;
}

inline constexpr D3D12_STENCIL_OP toD3D12StencilOp(RHIStencilOp op) noexcept
{
	switch (op) {
	case RHIStencilOp::Keep:
		return D3D12_STENCIL_OP_KEEP;
	case RHIStencilOp::Zero:
		return D3D12_STENCIL_OP_ZERO;
	case RHIStencilOp::Replace:
		return D3D12_STENCIL_OP_REPLACE;
	case RHIStencilOp::IncrementAndClamp:
		return D3D12_STENCIL_OP_INCR_SAT;
	case RHIStencilOp::DecrementAndClamp:
		return D3D12_STENCIL_OP_DECR_SAT;
	case RHIStencilOp::Invert:
		return D3D12_STENCIL_OP_INVERT;
	case RHIStencilOp::IncrementAndWrap:
		return D3D12_STENCIL_OP_INCR;
	case RHIStencilOp::DecrementAndWrap:
		return D3D12_STENCIL_OP_DECR;
	}

	return D3D12_STENCIL_OP_KEEP;
}

inline constexpr D3D12_DEPTH_STENCILOP_DESC toD3D12DepthStencilOpDesc(const RHIDepthStencilState::StencilOp& op)
{
	return {toD3D12StencilOp(op.fail_op), toD3D12StencilOp(op.depth_fail_op),
	    toD3D12StencilOp(op.pass_op), toD3D12ComparisonFunc(op.stencil_func)};
}

inline constexpr D3D12_FILL_MODE toD3D12FillMode(RHIPolygonMode mode) noexcept
{
	switch (mode) {
	case RHIPolygonMode::Fill:
		return D3D12_FILL_MODE_SOLID;
	case RHIPolygonMode::Line:
		return D3D12_FILL_MODE_WIREFRAME;
	case RHIPolygonMode::Point:
		break;
	}

	return D3D12_FILL_MODE_SOLID;
}

inline constexpr D3D12_CULL_MODE toD3D12CullMode(RHICullMode mode) noexcept
{
	switch (mode) {
	case RHICullMode::None:
		return D3D12_CULL_MODE_NONE;
	case RHICullMode::Front:
		return D3D12_CULL_MODE_FRONT;
	case RHICullMode::Back:
		return D3D12_CULL_MODE_BACK;
	}

	return D3D12_CULL_MODE_BACK;
}

inline constexpr D3D_PRIMITIVE_TOPOLOGY toD3D12PrimitiveTopology(RHIPrimitiveType type) noexcept
{
	switch (type) {
	case RHIPrimitiveType::TriangleList:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case RHIPrimitiveType::TriangleStrip:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case RHIPrimitiveType::LineList:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case RHIPrimitiveType::LineStrip:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case RHIPrimitiveType::PointList:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	}

	return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

inline constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE toD3D12PrimitiveTopologyType(RHIPrimitiveType type) noexcept
{
	switch (type) {
	case RHIPrimitiveType::TriangleList:
	case RHIPrimitiveType::TriangleStrip:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case RHIPrimitiveType::LineList:
	case RHIPrimitiveType::LineStrip:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case RHIPrimitiveType::PointList:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	}

	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

inline constexpr D3D12_FILTER toD3D12Filter(bool min_filter, bool mag_filter, bool mip_filter) noexcept
{
	if (min_filter) {
		if (mag_filter)
			return mip_filter ? D3D12_FILTER_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		return mip_filter ? D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR : D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	}

	if (mag_filter)
		return mip_filter ? D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR : D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
	return mip_filter ? D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_POINT;
}

inline constexpr D3D12_TEXTURE_ADDRESS_MODE toD3D12TextureAddressMode(RHISamplerAddress address)
{
	switch (address) {
	case RHISamplerAddress::Repeat:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case RHISamplerAddress::MirroredRepeat:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case RHISamplerAddress::ClampToEdge:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case RHISamplerAddress::ClampToBorder:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	}
	ERROR(Argument, "Invalid sampler address mode");
}

inline constexpr D3D12_RESOURCE_STATES toD3D12ResourceStates(RHIResourceState state) noexcept
{
	D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;
	if (state & (ConstantBuffer | VertexBuffer))
		result = result | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	if (state & IndexBuffer)
		result = result | D3D12_RESOURCE_STATE_INDEX_BUFFER;
	if (state & IndirectBuffer)
		result = result | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	if (state & RenderTarget)
		result = result | D3D12_RESOURCE_STATE_RENDER_TARGET;
	if (state & DepthWrite)
		result = result | D3D12_RESOURCE_STATE_DEPTH_WRITE;
	if (state & DepthRead)
		result = result | D3D12_RESOURCE_STATE_DEPTH_READ;
	if (state & ShaderResource)
		result = result | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	if (state & UnorderedAccess)
		result = result | D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	if (state & TransferSource)
		result = result | D3D12_RESOURCE_STATE_COPY_SOURCE;
	if (state & TransferDest)
		result = result | D3D12_RESOURCE_STATE_COPY_DEST;
	return result;
}

inline constexpr D3D12_HEAP_TYPE toD3D12HeapType(RHIAccessMode access) noexcept
{
	switch (access) {
	case RHIAccessMode::None:
		return D3D12_HEAP_TYPE_DEFAULT;
	case RHIAccessMode::Read:
		return D3D12_HEAP_TYPE_READBACK;
	case RHIAccessMode::Write:
		return D3D12_HEAP_TYPE_UPLOAD;
	}

	return D3D12_HEAP_TYPE_DEFAULT;
}

inline constexpr D3D12_RESOURCE_STATES getD3D12InitialBufferState(RHIAccessMode access) noexcept
{
	switch (access) {
	case RHIAccessMode::None:
		return D3D12_RESOURCE_STATE_COMMON;
	case RHIAccessMode::Read:
		return D3D12_RESOURCE_STATE_COPY_DEST;
	case RHIAccessMode::Write:
		return D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	return D3D12_RESOURCE_STATE_COMMON;
}

inline constexpr D3D12_RESOURCE_DIMENSION toD3D12ResourceDimension(RHITextureDimension dimension) noexcept
{
	switch (dimension) {
	case RHITextureDimension::Texture1D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	case RHITextureDimension::Texture2D:
	case RHITextureDimension::TextureCube:
		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	case RHITextureDimension::Texture3D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	}

	return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

inline constexpr D3D12_RESOURCE_FLAGS toD3D12ResourceFlags(RHITextureUsage usage) noexcept
{
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	if ((usage & RHITextureUsage::RenderTarget) != RHITextureUsage::None)
		flags = flags | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if ((usage & RHITextureUsage::DepthStencil) != RHITextureUsage::None)
		flags = flags | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if ((usage & RHITextureUsage::Storage) != RHITextureUsage::None)
		flags = flags | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	return flags;
}

inline constexpr D3D12_RESOURCE_DESC toD3D12BufferDesc(uint64 size) noexcept
{
	return {
	    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	    .Width = size,
	    .Height = 1,
	    .DepthOrArraySize = 1,
	    .MipLevels = 1,
	    .SampleDesc = {.Count = 1},
	    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	};
}

inline constexpr D3D12_RESOURCE_DESC toD3D12TextureDesc(const RHITextureDesc& desc)
{
	return {
	    .Dimension = toD3D12ResourceDimension(desc.dimension),
	    .Width = desc.width,
	    .Height = desc.height,
	    .DepthOrArraySize = static_cast<UINT16>(desc.dimension == RHITextureDimension::Texture3D ? desc.depth : desc.array_layers),
	    .MipLevels = static_cast<UINT16>(desc.mip_levels),
	    .Format = toD3D12ResourceFormat(desc.format),
	    .SampleDesc = {.Count = desc.sample_count},
	    .Flags = toD3D12ResourceFlags(desc.usage),
	};
}

inline constexpr D3D12_DESCRIPTOR_RANGE_TYPE toD3D12DescriptorRangeType(RHIBindingType type) noexcept
{
	switch (type) {
	case RHIBindingType::Sampler:
		return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	case RHIBindingType::ConstantBuffer:
		return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	case RHIBindingType::TextureUAV:
	case RHIBindingType::TypedBufferUAV:
	case RHIBindingType::StructuredBufferUAV:
	case RHIBindingType::RawBufferUAV:
		return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	default:
		return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	}
}

inline constexpr D3D12_VIEWPORT toD3D12Viewport(const RHIViewport& viewport) noexcept
{
	return {viewport.x_min, viewport.y_min, viewport.x_max - viewport.x_min,
	    viewport.y_max - viewport.y_min, viewport.z_min, viewport.z_max};
}

inline constexpr D3D12_RECT toD3D12Rect(const RHIRect& rect) noexcept
{
	return {rect.x_min, rect.y_min, rect.x_max, rect.y_max};
}

inline constexpr D3D12_RENDER_TARGET_VIEW_DESC toD3D12RenderTargetViewDesc(const RHITextureViewDesc& desc, uint32 sample_count)
{
	const auto& sub = desc.subresource;
	const auto dimension = desc.dimension;
	const bool ms = sample_count > 1;

	D3D12_RENDER_TARGET_VIEW_DESC view{};
	view.Format = toDXGIFormat(desc.format);
	switch (dimension) {
	case RHITextureViewDimension::Texture1D:
		view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		view.Texture1D.MipSlice = sub.base_mip_level;
		break;
	case RHITextureViewDimension::Texture1DArray:
		view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		view.Texture1DArray = {sub.base_mip_level, sub.base_array_layer, sub.layer_count};
		break;
	case RHITextureViewDimension::Texture2D:
		view.ViewDimension = ms ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
		if (!ms)
			view.Texture2D.MipSlice = sub.base_mip_level;
		break;
	case RHITextureViewDimension::Texture2DArray:
		view.ViewDimension = ms ? D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		if (ms)
			view.Texture2DMSArray = {sub.base_array_layer, sub.layer_count};
		else
			view.Texture2DArray = {sub.base_mip_level, sub.base_array_layer, sub.layer_count, 0};
		break;
	case RHITextureViewDimension::Texture3D:
		view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		view.Texture3D = {sub.base_mip_level, 0, ~0u};
		break;
	default:
		ERROR(Argument, "Invalid D3D12 RTV dimension");
	}

	return view;
}

inline constexpr D3D12_DEPTH_STENCIL_VIEW_DESC toD3D12DepthStencilViewDesc(const RHITextureViewDesc& desc, uint32 sample_count)
{
	const auto& sub = desc.subresource;
	const auto dimension = desc.dimension;
	const bool ms = sample_count > 1;

	D3D12_DEPTH_STENCIL_VIEW_DESC view{};
	view.Format = toDXGIFormat(desc.format);
	switch (dimension) {
	case RHITextureViewDimension::Texture1D:
		view.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		view.Texture1D.MipSlice = sub.base_mip_level;
		break;
	case RHITextureViewDimension::Texture1DArray:
		view.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		view.Texture1DArray = {sub.base_mip_level, sub.base_array_layer, sub.layer_count};
		break;
	case RHITextureViewDimension::Texture2D:
		view.ViewDimension = ms ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
		if (!ms)
			view.Texture2D.MipSlice = sub.base_mip_level;
		break;
	case RHITextureViewDimension::Texture2DArray:
		view.ViewDimension = ms ? D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY : D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		if (ms)
			view.Texture2DMSArray = {sub.base_array_layer, sub.layer_count};
		else
			view.Texture2DArray = {sub.base_mip_level, sub.base_array_layer, sub.layer_count};
		break;
	default:
		ERROR(Argument, "Invalid D3D12 DSV dimension");
	}

	return view;
}

inline constexpr D3D12_SHADER_RESOURCE_VIEW_DESC toD3D12ShaderResourceViewDesc(const RHITextureViewDesc& desc, uint32 sample_count)
{
	const auto& sub = desc.subresource;
	const auto dimension = desc.dimension;
	const bool ms = sample_count > 1;

	D3D12_SHADER_RESOURCE_VIEW_DESC view{};
	view.Format = toD3D12SRVFormat(desc.format);
	view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	switch (dimension) {
	case RHITextureViewDimension::Texture1D:
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		view.Texture1D = {sub.base_mip_level, sub.level_count, 0};
		break;
	case RHITextureViewDimension::Texture1DArray:
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		view.Texture1DArray = {sub.base_mip_level, sub.level_count, sub.base_array_layer, sub.layer_count, 0};
		break;
	case RHITextureViewDimension::Texture2D:
		view.ViewDimension = ms ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
		if (!ms)
			view.Texture2D = {sub.base_mip_level, sub.level_count, 0, 0};
		break;
	case RHITextureViewDimension::Texture2DArray:
		view.ViewDimension = ms ? D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		if (ms)
			view.Texture2DMSArray = {sub.base_array_layer, sub.layer_count};
		else
			view.Texture2DArray = {sub.base_mip_level, sub.level_count, sub.base_array_layer, sub.layer_count, 0, 0};
		break;
	case RHITextureViewDimension::Texture3D:
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		view.Texture3D = {sub.base_mip_level, sub.level_count, 0};
		break;
	case RHITextureViewDimension::TextureCube:
		CHECK(sub.base_array_layer == 0, "D3D12 TextureCube SRVs must start at face zero; use TextureCubeArray to select other cubes");
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		view.TextureCube = {sub.base_mip_level, sub.level_count, 0};
		break;
	case RHITextureViewDimension::TextureCubeArray:
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		view.TextureCubeArray = {sub.base_mip_level, sub.level_count, sub.base_array_layer, sub.layer_count / 6, 0};
		break;
	default:
		ERROR(Argument, "Invalid D3D12 SRV dimension");
	}

	return view;
}

inline constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC toD3D12UnorderedAccessViewDesc(const RHITextureViewDesc& desc)
{
	const auto& sub = desc.subresource;
	const auto dimension = desc.dimension;

	D3D12_UNORDERED_ACCESS_VIEW_DESC view{};
	view.Format = toDXGIFormat(desc.format);
	switch (dimension) {
	case RHITextureViewDimension::Texture1D:
		view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		view.Texture1D.MipSlice = sub.base_mip_level;
		break;
	case RHITextureViewDimension::Texture1DArray:
		view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		view.Texture1DArray = {sub.base_mip_level, sub.base_array_layer, sub.layer_count};
		break;
	case RHITextureViewDimension::Texture2D:
		view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		view.Texture2D.MipSlice = sub.base_mip_level;
		break;
	case RHITextureViewDimension::Texture2DArray:
		view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		view.Texture2DArray = {sub.base_mip_level, sub.base_array_layer, sub.layer_count, 0};
		break;
	case RHITextureViewDimension::Texture3D:
		view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		view.Texture3D = {sub.base_mip_level, 0, ~0u};
		break;
	default:
		ERROR(Argument, "Invalid D3D12 UAV dimension");
	}

	return view;
}

inline constexpr D3D12_CONSTANT_BUFFER_VIEW_DESC toD3D12ConstantBufferViewDesc(const RHIBufferViewDesc& desc, D3D12_GPU_VIRTUAL_ADDRESS address)
{
	constexpr uint64 alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	return {address + desc.offset, static_cast<UINT>((desc.size + alignment - 1) & ~(alignment - 1))};
}

inline constexpr uint32 getD3D12BufferViewStride(const RHIBufferViewDesc& desc)
{
	const uint32 stride = desc.type == RHIBufferViewType::Raw ? 4 :
	    desc.type == RHIBufferViewType::Structured            ? desc.stride :
	                                                            getRHIFormatByteSize(desc.format);
	CHECK(stride && desc.size / stride <= ~uint32(0), "D3D12 buffer view element count exceeds UINT32_MAX");

	return stride;
}

inline constexpr D3D12_SHADER_RESOURCE_VIEW_DESC toD3D12ShaderResourceViewDesc(const RHIBufferViewDesc& desc)
{
	const bool raw = desc.type == RHIBufferViewType::Raw;
	const bool structured = desc.type == RHIBufferViewType::Structured;
	const uint32 stride = getD3D12BufferViewStride(desc);
	const auto format = raw ? DXGI_FORMAT_R32_TYPELESS : structured ? DXGI_FORMAT_UNKNOWN :
	                                                                  toDXGIFormat(desc.format);

	D3D12_SHADER_RESOURCE_VIEW_DESC native{};
	native.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	native.Format = format;
	native.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	native.Buffer.FirstElement = desc.offset / stride;
	native.Buffer.NumElements = static_cast<UINT>(desc.size / stride);
	native.Buffer.StructureByteStride = structured ? stride : 0;
	native.Buffer.Flags = raw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

	return native;
}

inline constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC toD3D12UnorderedAccessViewDesc(const RHIBufferViewDesc& desc)
{
	const bool raw = desc.type == RHIBufferViewType::Raw;
	const bool structured = desc.type == RHIBufferViewType::Structured;
	const uint32 stride = getD3D12BufferViewStride(desc);
	const auto format = raw ? DXGI_FORMAT_R32_TYPELESS : structured ? DXGI_FORMAT_UNKNOWN :
	                                                                  toDXGIFormat(desc.format);

	D3D12_UNORDERED_ACCESS_VIEW_DESC native{};
	native.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	native.Format = format;
	native.Buffer.FirstElement = desc.offset / stride;
	native.Buffer.NumElements = static_cast<UINT>(desc.size / stride);
	native.Buffer.StructureByteStride = structured ? stride : 0;
	native.Buffer.Flags = raw ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

	return native;
}

inline constexpr D3D12_SAMPLER_DESC toD3D12SamplerDesc(const RHISamplerDesc& desc)
{
	return {
	    .Filter = toD3D12Filter(desc.min_filter, desc.mag_filter, desc.mip_filter),
	    .AddressU = toD3D12TextureAddressMode(desc.address_u),
	    .AddressV = toD3D12TextureAddressMode(desc.address_v),
	    .AddressW = toD3D12TextureAddressMode(desc.address_w),
	    .MipLODBias = desc.mip_bias,
	    .MaxAnisotropy = 1,
	    .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
	    .BorderColor = {0, 0, 0, 1},
	    .MaxLOD = D3D12_FLOAT32_MAX,
	};
}

inline constexpr D3D12_CLEAR_VALUE toD3D12ClearValue(const RHITextureDesc& desc)
{
	D3D12_CLEAR_VALUE clear{};
	clear.Format = toDXGIFormat(desc.format);
	if (isRHIDepthFormat(desc.format))
		clear.DepthStencil.Depth = 1.0f;
	else {
		clear.Color[0] = desc.clear_color.r;
		clear.Color[1] = desc.clear_color.g;
		clear.Color[2] = desc.clear_color.b;
		clear.Color[3] = desc.clear_color.a;
	}

	return clear;
}

inline constexpr D3D12_BLEND_DESC toD3D12BlendDesc(const RHIColorBlendState& desc)
{
	D3D12_BLEND_DESC native{
	    .AlphaToCoverageEnable = desc.alpha_to_coverage_enable,
	    .IndependentBlendEnable = true,
	};
	for (uint32 i = 0; i < 8; ++i) {
		const auto blend = i < desc.blend_descs.size() ? desc.blend_descs[i] : RHIColorBlendState::BlendDesc{};
		auto& target = native.RenderTarget[i];
		target.BlendEnable = blend.blend_enable;
		target.SrcBlend = toD3D12Blend(blend.src_blend);
		target.DestBlend = toD3D12Blend(blend.dst_blend);
		target.BlendOp = toD3D12BlendOp(blend.color_blend_op);
		target.SrcBlendAlpha = toD3D12Blend(blend.src_blend_alpha);
		target.DestBlendAlpha = toD3D12Blend(blend.dst_blend_alpha);
		target.BlendOpAlpha = toD3D12BlendOp(blend.alpha_blend_op);
		target.LogicOp = D3D12_LOGIC_OP_NOOP;
		target.RenderTargetWriteMask = toD3D12ColorWriteMask(blend.color_write_mask);
	}

	return native;
}

inline constexpr D3D12_RASTERIZER_DESC toD3D12RasterizerDesc(const RHIRasterState& desc, uint32 sample_count)
{
	return {
	    .FillMode = toD3D12FillMode(desc.polygon_mode),
	    .CullMode = toD3D12CullMode(desc.cull_mode),
	    .FrontCounterClockwise = desc.front_face == RHIFrontFace::CounterClockwise,
	    .DepthBias = desc.depth_bias,
	    .DepthBiasClamp = desc.depth_bias_clamp,
	    .SlopeScaledDepthBias = desc.depth_bias_slope_factor,
	    .DepthClipEnable = !desc.depth_clamp_enable,
	    .MultisampleEnable = sample_count > 1,
	};
}

inline constexpr D3D12_DEPTH_STENCIL_DESC toD3D12DepthStencilDesc(const RHIDepthStencilState& desc)
{
	return {
	    .DepthEnable = desc.depth_test_enable,
	    .DepthWriteMask = desc.depth_write_enable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
	    .DepthFunc = toD3D12ComparisonFunc(desc.depth_compare_op),
	    .StencilEnable = desc.stencil_test_enable,
	    .StencilReadMask = 0xff,
	    .StencilWriteMask = 0xff,
	    .FrontFace = toD3D12DepthStencilOpDesc(desc.front_face_stencil),
	    .BackFace = toD3D12DepthStencilOpDesc(desc.back_face_stencil),
	};
}

inline void setD3D12Name(ID3D12Object* object, const std::string& name) noexcept
{
	if (!object || name.empty())
		return;

	const int length = MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), nullptr, 0);
	std::wstring wide(length, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), wide.data(), length);
	object->SetName(wide.c_str());
}

}        // namespace Vortex

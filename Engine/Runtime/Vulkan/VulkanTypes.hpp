export module Runtime.Vulkan:Types;

import vulkan;
import Core;
import Runtime.RHI;

export namespace Vortex {

struct VulkanTextureTransition {
	vk::ImageLayout layout{};
	vk::PipelineStageFlags2 stage{};
	vk::AccessFlags2 access{};
};

struct VulkanBufferTransition {
	vk::PipelineStageFlags2 stage{};
	vk::AccessFlags2 access{};
};

inline constexpr vk::Format toVkFormat(RHIFormat format) noexcept
{
	switch (format) {
	case RHIFormat::Unknown:
		return vk::Format::eUndefined;
	case RHIFormat::R8_UINT:
		return vk::Format::eR8Uint;
	case RHIFormat::R8_SINT:
		return vk::Format::eR8Sint;
	case RHIFormat::R8_UNORM:
		return vk::Format::eR8Unorm;
	case RHIFormat::R8_SNORM:
		return vk::Format::eR8Snorm;
	case RHIFormat::RG8_UINT:
		return vk::Format::eR8G8Uint;
	case RHIFormat::RG8_SINT:
		return vk::Format::eR8G8Sint;
	case RHIFormat::RG8_UNORM:
		return vk::Format::eR8G8Unorm;
	case RHIFormat::RG8_SNORM:
		return vk::Format::eR8G8Snorm;
	case RHIFormat::RGBA8_UINT:
		return vk::Format::eR8G8B8A8Uint;
	case RHIFormat::RGBA8_SINT:
		return vk::Format::eR8G8B8A8Sint;
	case RHIFormat::RGBA8_UNORM:
		return vk::Format::eR8G8B8A8Unorm;
	case RHIFormat::RGBA8_SRGB:
		return vk::Format::eR8G8B8A8Srgb;
	case RHIFormat::RGBA8_SNORM:
		return vk::Format::eR8G8B8A8Snorm;
	case RHIFormat::BGRA8_UNORM:
		return vk::Format::eB8G8R8A8Unorm;
	case RHIFormat::BGRA8_SRGB:
		return vk::Format::eB8G8R8A8Srgb;
	case RHIFormat::R16_UINT:
		return vk::Format::eR16Uint;
	case RHIFormat::R16_SINT:
		return vk::Format::eR16Sint;
	case RHIFormat::R16_UNORM:
		return vk::Format::eR16Unorm;
	case RHIFormat::R16_SNORM:
		return vk::Format::eR16Snorm;
	case RHIFormat::R16_FLOAT:
		return vk::Format::eR16Sfloat;
	case RHIFormat::RG16_UINT:
		return vk::Format::eR16G16Uint;
	case RHIFormat::RG16_SINT:
		return vk::Format::eR16G16Sint;
	case RHIFormat::RG16_UNORM:
		return vk::Format::eR16G16Unorm;
	case RHIFormat::RG16_SNORM:
		return vk::Format::eR16G16Snorm;
	case RHIFormat::RG16_FLOAT:
		return vk::Format::eR16G16Sfloat;
	case RHIFormat::RGBA16_UINT:
		return vk::Format::eR16G16B16A16Uint;
	case RHIFormat::RGBA16_SINT:
		return vk::Format::eR16G16B16A16Sint;
	case RHIFormat::RGBA16_FLOAT:
		return vk::Format::eR16G16B16A16Sfloat;
	case RHIFormat::RGBA16_UNORM:
		return vk::Format::eR16G16B16A16Unorm;
	case RHIFormat::RGBA16_SNORM:
		return vk::Format::eR16G16B16A16Snorm;
	case RHIFormat::R32_UINT:
		return vk::Format::eR32Uint;
	case RHIFormat::R32_SINT:
		return vk::Format::eR32Sint;
	case RHIFormat::R32_FLOAT:
		return vk::Format::eR32Sfloat;
	case RHIFormat::RG32_UINT:
		return vk::Format::eR32G32Uint;
	case RHIFormat::RG32_SINT:
		return vk::Format::eR32G32Sint;
	case RHIFormat::RG32_FLOAT:
		return vk::Format::eR32G32Sfloat;
	case RHIFormat::RGB32_UINT:
		return vk::Format::eR32G32B32Uint;
	case RHIFormat::RGB32_SINT:
		return vk::Format::eR32G32B32Sint;
	case RHIFormat::RGB32_FLOAT:
		return vk::Format::eR32G32B32Sfloat;
	case RHIFormat::RGBA32_UINT:
		return vk::Format::eR32G32B32A32Uint;
	case RHIFormat::RGBA32_SINT:
		return vk::Format::eR32G32B32A32Sint;
	case RHIFormat::RGBA32_FLOAT:
		return vk::Format::eR32G32B32A32Sfloat;
	case RHIFormat::D16_UNORM:
		return vk::Format::eD16Unorm;
	case RHIFormat::D24_UNORM_S8_UINT:
		return vk::Format::eD24UnormS8Uint;
	case RHIFormat::D32_FLOAT:
		return vk::Format::eD32Sfloat;
	}

	return vk::Format::eUndefined;
}

inline constexpr vk::DescriptorType toVkDescriptorType(RHIBindingType type) noexcept
{
	switch (type) {
	case RHIBindingType::TextureSRV:
		return vk::DescriptorType::eSampledImage;
	case RHIBindingType::TextureUAV:
		return vk::DescriptorType::eStorageImage;
	case RHIBindingType::TypedBufferSRV:
		return vk::DescriptorType::eUniformTexelBuffer;
	case RHIBindingType::TypedBufferUAV:
		return vk::DescriptorType::eStorageTexelBuffer;
	case RHIBindingType::StructuredBufferSRV:
	case RHIBindingType::StructuredBufferUAV:
	case RHIBindingType::RawBufferSRV:
	case RHIBindingType::RawBufferUAV:
		return vk::DescriptorType::eStorageBuffer;
	case RHIBindingType::ConstantBuffer:
		return vk::DescriptorType::eUniformBuffer;
	case RHIBindingType::Sampler:
		return vk::DescriptorType::eSampler;
	default:
		return static_cast<vk::DescriptorType>(0x7FFFFFFF);
	}
}

inline constexpr vk::ColorComponentFlags toVkColorComponentFlags(RHIColorMask mask) noexcept
{
	vk::ColorComponentFlags flags{};
	if ((mask & RHIColorMask::R) != RHIColorMask::None)
		flags |= vk::ColorComponentFlagBits::eR;
	if ((mask & RHIColorMask::G) != RHIColorMask::None)
		flags |= vk::ColorComponentFlagBits::eG;
	if ((mask & RHIColorMask::B) != RHIColorMask::None)
		flags |= vk::ColorComponentFlagBits::eB;
	if ((mask & RHIColorMask::A) != RHIColorMask::None)
		flags |= vk::ColorComponentFlagBits::eA;

	return flags;
}

inline constexpr vk::IndexType toVkIndexType(RHIFormat format) noexcept
{
	switch (format) {
	case RHIFormat::R16_UINT:
		return vk::IndexType::eUint16;
	case RHIFormat::R32_UINT:
		return vk::IndexType::eUint32;
	default:
		return vk::IndexType::eNoneKHR;
	}
}

inline constexpr vk::PrimitiveTopology toVkPrimitiveTopology(RHIPrimitiveType type) noexcept
{
	switch (type) {
	case RHIPrimitiveType::TriangleList:
		return vk::PrimitiveTopology::eTriangleList;
	case RHIPrimitiveType::TriangleStrip:
		return vk::PrimitiveTopology::eTriangleStrip;
	case RHIPrimitiveType::LineList:
		return vk::PrimitiveTopology::eLineList;
	case RHIPrimitiveType::LineStrip:
		return vk::PrimitiveTopology::eLineStrip;
	case RHIPrimitiveType::PointList:
		return vk::PrimitiveTopology::ePointList;
	}

	return vk::PrimitiveTopology::eTriangleList;
}

inline constexpr vk::CompareOp toVkCompareOp(RHICompareOp op) noexcept
{
	switch (op) {
	case RHICompareOp::Never:
		return vk::CompareOp::eNever;
	case RHICompareOp::Less:
		return vk::CompareOp::eLess;
	case RHICompareOp::LessOrEqual:
		return vk::CompareOp::eLessOrEqual;
	case RHICompareOp::Greater:
		return vk::CompareOp::eGreater;
	case RHICompareOp::NotEqual:
		return vk::CompareOp::eNotEqual;
	case RHICompareOp::GreaterOrEqual:
		return vk::CompareOp::eGreaterOrEqual;
	case RHICompareOp::Always:
		return vk::CompareOp::eAlways;
	}

	return vk::CompareOp::eAlways;
}

inline constexpr vk::BlendFactor toVkBlendFactor(RHIBlendFactor factor) noexcept
{
	switch (factor) {
	case RHIBlendFactor::Zero:
		return vk::BlendFactor::eZero;
	case RHIBlendFactor::One:
		return vk::BlendFactor::eOne;
	case RHIBlendFactor::SrcColor:
		return vk::BlendFactor::eSrcColor;
	case RHIBlendFactor::OneMinusSrcColor:
		return vk::BlendFactor::eOneMinusSrcColor;
	case RHIBlendFactor::DstColor:
		return vk::BlendFactor::eDstColor;
	case RHIBlendFactor::OneMinusDstColor:
		return vk::BlendFactor::eOneMinusDstColor;
	case RHIBlendFactor::SrcAlpha:
		return vk::BlendFactor::eSrcAlpha;
	case RHIBlendFactor::OneMinusSrcAlpha:
		return vk::BlendFactor::eOneMinusSrcAlpha;
	case RHIBlendFactor::DstAlpha:
		return vk::BlendFactor::eDstAlpha;
	case RHIBlendFactor::OneMinusDstAlpha:
		return vk::BlendFactor::eOneMinusDstAlpha;
	}

	return vk::BlendFactor::eOne;
}

inline constexpr vk::BlendOp toVkBlendOp(RHIBlendOp op) noexcept
{
	switch (op) {
	case RHIBlendOp::Add:
		return vk::BlendOp::eAdd;
	case RHIBlendOp::Subtract:
		return vk::BlendOp::eSubtract;
	case RHIBlendOp::ReverseSubtract:
		return vk::BlendOp::eReverseSubtract;
	case RHIBlendOp::Min:
		return vk::BlendOp::eMin;
	case RHIBlendOp::Max:
		return vk::BlendOp::eMax;
	}

	return vk::BlendOp::eAdd;
}

inline constexpr vk::StencilOp toVkStencilOp(RHIStencilOp op) noexcept
{
	switch (op) {
	case RHIStencilOp::Keep:
		return vk::StencilOp::eKeep;
	case RHIStencilOp::Zero:
		return vk::StencilOp::eZero;
	case RHIStencilOp::Replace:
		return vk::StencilOp::eReplace;
	case RHIStencilOp::IncrementAndClamp:
		return vk::StencilOp::eIncrementAndClamp;
	case RHIStencilOp::DecrementAndClamp:
		return vk::StencilOp::eDecrementAndClamp;
	case RHIStencilOp::Invert:
		return vk::StencilOp::eInvert;
	case RHIStencilOp::IncrementAndWrap:
		return vk::StencilOp::eIncrementAndWrap;
	case RHIStencilOp::DecrementAndWrap:
		return vk::StencilOp::eDecrementAndWrap;
	}

	return vk::StencilOp::eKeep;
}

inline constexpr vk::PolygonMode toVkPolygonMode(RHIPolygonMode mode) noexcept
{
	switch (mode) {
	case RHIPolygonMode::Fill:
		return vk::PolygonMode::eFill;
	case RHIPolygonMode::Line:
		return vk::PolygonMode::eLine;
	case RHIPolygonMode::Point:
		return vk::PolygonMode::ePoint;
	}

	return vk::PolygonMode::eFill;
}

inline constexpr vk::CullModeFlags toVkCullMode(RHICullMode mode) noexcept
{
	switch (mode) {
	case RHICullMode::None:
		return vk::CullModeFlagBits::eNone;
	case RHICullMode::Front:
		return vk::CullModeFlagBits::eFront;
	case RHICullMode::Back:
		return vk::CullModeFlagBits::eBack;
	}

	return vk::CullModeFlagBits::eBack;
}

inline constexpr vk::FrontFace toVkFrontFace(RHIFrontFace face) noexcept
{
	switch (face) {
	case RHIFrontFace::CounterClockwise:
		return vk::FrontFace::eCounterClockwise;
	case RHIFrontFace::Clockwise:
		return vk::FrontFace::eClockwise;
	}

	return vk::FrontFace::eCounterClockwise;
}

inline constexpr vk::ImageType toVkImageType(RHITextureDimension dimension) noexcept
{
	switch (dimension) {
	case RHITextureDimension::Texture1D:
		return vk::ImageType::e1D;
	case RHITextureDimension::Texture2D:
	case RHITextureDimension::TextureCube:
		return vk::ImageType::e2D;
	case RHITextureDimension::Texture3D:
		return vk::ImageType::e3D;
	}

	return vk::ImageType::e2D;
}

inline constexpr vk::ImageViewType toVkImageViewType(RHITextureDimension dimension) noexcept
{
	switch (dimension) {
	case RHITextureDimension::Texture1D:
		return vk::ImageViewType::e1D;
	case RHITextureDimension::Texture2D:
		return vk::ImageViewType::e2D;
	case RHITextureDimension::Texture3D:
		return vk::ImageViewType::e3D;
	case RHITextureDimension::TextureCube:
		return vk::ImageViewType::eCube;
	}

	return vk::ImageViewType::e2D;
}

inline constexpr vk::ImageViewType toVkImageViewType(RHITextureViewDimension dimension) noexcept
{
	switch (dimension) {
	case RHITextureViewDimension::Texture1D:
		return vk::ImageViewType::e1D;
	case RHITextureViewDimension::Texture1DArray:
		return vk::ImageViewType::e1DArray;
	case RHITextureViewDimension::Texture2D:
		return vk::ImageViewType::e2D;
	case RHITextureViewDimension::Texture2DArray:
		return vk::ImageViewType::e2DArray;
	case RHITextureViewDimension::Texture3D:
		return vk::ImageViewType::e3D;
	case RHITextureViewDimension::TextureCube:
		return vk::ImageViewType::eCube;
	case RHITextureViewDimension::TextureCubeArray:
		return vk::ImageViewType::eCubeArray;
	case RHITextureViewDimension::Automatic:
		break;
	}

	return vk::ImageViewType::e2D;
}

inline constexpr vk::ImageUsageFlagBits toVkImageUsageFlagBits(RHITextureUsage usage) noexcept
{
	switch (usage) {
	case RHITextureUsage::Sampled:
		return vk::ImageUsageFlagBits::eSampled;
	case RHITextureUsage::RenderTarget:
		return vk::ImageUsageFlagBits::eColorAttachment;
	case RHITextureUsage::DepthStencil:
		return vk::ImageUsageFlagBits::eDepthStencilAttachment;
	case RHITextureUsage::Storage:
		return vk::ImageUsageFlagBits::eStorage;
	case RHITextureUsage::CopySource:
		return vk::ImageUsageFlagBits::eTransferSrc;
	case RHITextureUsage::CopyDest:
		return vk::ImageUsageFlagBits::eTransferDst;
	default:
		return vk::ImageUsageFlagBits::eSampled;
	}
}

inline constexpr vk::ImageUsageFlags toVkImageUsageFlags(RHITextureUsage usage) noexcept
{
	vk::ImageUsageFlags flags{};
	if ((usage & RHITextureUsage::Sampled) != RHITextureUsage::None)
		flags |= vk::ImageUsageFlagBits::eSampled;
	if ((usage & RHITextureUsage::RenderTarget) != RHITextureUsage::None)
		flags |= vk::ImageUsageFlagBits::eColorAttachment;
	if ((usage & RHITextureUsage::DepthStencil) != RHITextureUsage::None)
		flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
	if ((usage & RHITextureUsage::Storage) != RHITextureUsage::None)
		flags |= vk::ImageUsageFlagBits::eStorage;
	if ((usage & RHITextureUsage::CopySource) != RHITextureUsage::None)
		flags |= vk::ImageUsageFlagBits::eTransferSrc;
	if ((usage & RHITextureUsage::CopyDest) != RHITextureUsage::None)
		flags |= vk::ImageUsageFlagBits::eTransferDst;

	return flags;
}

inline constexpr vk::SamplerAddressMode toVkSamplerAddressMode(RHISamplerAddress address) noexcept
{
	switch (address) {
	case RHISamplerAddress::Repeat:
		return vk::SamplerAddressMode::eRepeat;
	case RHISamplerAddress::MirroredRepeat:
		return vk::SamplerAddressMode::eMirroredRepeat;
	case RHISamplerAddress::ClampToEdge:
		return vk::SamplerAddressMode::eClampToEdge;
	case RHISamplerAddress::ClampToBorder:
		return vk::SamplerAddressMode::eClampToBorder;
	}

	return vk::SamplerAddressMode::eRepeat;
}

inline constexpr vk::ShaderStageFlagBits toVkShaderStageFlagBits(RHIShaderType type) noexcept
{
	switch (type) {
	case RHIShaderType::Vertex:
		return vk::ShaderStageFlagBits::eVertex;
	case RHIShaderType::Pixel:
		return vk::ShaderStageFlagBits::eFragment;
	case RHIShaderType::Geometry:
		return vk::ShaderStageFlagBits::eGeometry;
	case RHIShaderType::Compute:
		return vk::ShaderStageFlagBits::eCompute;
	case RHIShaderType::AllGraphics:
		return vk::ShaderStageFlagBits::eAllGraphics;
	case RHIShaderType::All:
		return vk::ShaderStageFlagBits::eAll;
	default:
		return vk::ShaderStageFlagBits::eAll;
	}
}

inline constexpr vk::ShaderStageFlags toVkShaderStageFlags(RHIShaderType type) noexcept
{
	vk::ShaderStageFlags flags{};
	if ((type & RHIShaderType::Vertex) != RHIShaderType::None)
		flags |= vk::ShaderStageFlagBits::eVertex;
	if ((type & RHIShaderType::Pixel) != RHIShaderType::None)
		flags |= vk::ShaderStageFlagBits::eFragment;
	if ((type & RHIShaderType::Geometry) != RHIShaderType::None)
		flags |= vk::ShaderStageFlagBits::eGeometry;
	if ((type & RHIShaderType::Compute) != RHIShaderType::None)
		flags |= vk::ShaderStageFlagBits::eCompute;

	return flags;
}

inline constexpr vk::BufferUsageFlags toVkBufferUsageFlags(RHIBufferUsage usage) noexcept
{
	vk::BufferUsageFlags flags{};
	if ((usage & RHIBufferUsage::VertexBuffer) != RHIBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eVertexBuffer;
	if ((usage & RHIBufferUsage::IndexBuffer) != RHIBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eIndexBuffer;
	if ((usage & RHIBufferUsage::ConstantBuffer) != RHIBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eUniformBuffer;
	if ((usage & RHIBufferUsage::StorageBuffer) != RHIBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eStorageBuffer;
	if ((usage & RHIBufferUsage::IndirectArgument) != RHIBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eIndirectBuffer;
	if ((usage & RHIBufferUsage::CopySource) != RHIBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eTransferSrc;
	if ((usage & RHIBufferUsage::CopyDest) != RHIBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eTransferDst;
	if ((usage & RHIBufferUsage::TypedBuffer) != RHIBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer;
	return flags;
}

inline constexpr vk::Filter toVkFilter(RHIFilter filter) noexcept
{
	switch (filter) {
	case RHIFilter::Nearest:
		return vk::Filter::eNearest;
	case RHIFilter::Linear:
		return vk::Filter::eLinear;
	}

	return vk::Filter::eNearest;
}

inline constexpr vk::MemoryPropertyFlags toVkMemoryPropertyFlags(RHIAccessMode access) noexcept
{
	switch (access) {
	case RHIAccessMode::None:
		return vk::MemoryPropertyFlagBits::eDeviceLocal;
	case RHIAccessMode::Read:
		return vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached;
	case RHIAccessMode::Write:
		return vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	}

	return vk::MemoryPropertyFlagBits::eDeviceLocal;
}

inline constexpr vk::ClearColorValue toVkClearColorValue(const RHIColor& color) noexcept
{
	return vk::ClearColorValue(
	    std::array<float, 4>{
	        color.r,
	        color.g,
	        color.b,
	        color.a});
}

inline constexpr vk::Extent2D toVkExtent2D(const RHIExtent& extent) noexcept
{
	return vk::Extent2D(
	    extent.width,
	    extent.height);
}

inline constexpr vk::Viewport toVkViewport(const RHIViewport& viewport) noexcept
{
	return vk::Viewport(
	    viewport.x_min,
	    viewport.y_min,
	    viewport.x_max - viewport.x_min,
	    viewport.y_max - viewport.y_min,
	    viewport.z_min,
	    viewport.z_max);
}

inline constexpr vk::ClearValue toVkClearValue(const RHIClearValue& clear_value) noexcept
{
	return vk::ClearValue(clear_value.depth != 0.0f || clear_value.stencil != 0 ?
	        vk::ClearValue().setDepthStencil({clear_value.depth, clear_value.stencil}) :
	        vk::ClearValue().setColor(toVkClearColorValue(clear_value.color)));
}

inline constexpr vk::Rect2D toVkRect2D(const RHIRect& rect) noexcept
{
	return vk::Rect2D(
	    vk::Offset2D(rect.x_min, rect.y_min),
	    vk::Extent2D(rect.width(), rect.height()));
}

inline constexpr vk::SampleCountFlagBits toVkSampleCountFlagBits(uint32 count) noexcept
{
	switch (count) {
	case 1:
		return vk::SampleCountFlagBits::e1;
	case 2:
		return vk::SampleCountFlagBits::e2;
	case 4:
		return vk::SampleCountFlagBits::e4;
	case 8:
		return vk::SampleCountFlagBits::e8;
	case 16:
		return vk::SampleCountFlagBits::e16;
	case 32:
		return vk::SampleCountFlagBits::e32;
	case 64:
		return vk::SampleCountFlagBits::e64;
	default:
		return vk::SampleCountFlagBits::e1;
	}
}

inline constexpr vk::PipelineStageFlags2 getVkPipelineStageFlags(vk::ImageLayout layout) noexcept
{
	switch (layout) {
	case vk::ImageLayout::eTransferSrcOptimal:
	case vk::ImageLayout::eTransferDstOptimal:
		return vk::PipelineStageFlagBits2::eTransfer;
	case vk::ImageLayout::eColorAttachmentOptimal:
		return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
	case vk::ImageLayout::eDepthStencilAttachmentOptimal:
	case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
		return vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
	case vk::ImageLayout::eShaderReadOnlyOptimal:
	case vk::ImageLayout::eGeneral:
		return vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eComputeShader;
	case vk::ImageLayout::ePresentSrcKHR:
		return vk::PipelineStageFlagBits2::eAllCommands;
	case vk::ImageLayout::eUndefined:
	default:
		return vk::PipelineStageFlagBits2::eTopOfPipe;
	}
}

inline constexpr vk::ImageAspectFlags getVkImageAspectFlags(RHIFormat format) noexcept
{
	switch (format) {
	case RHIFormat::D16_UNORM:
	case RHIFormat::D32_FLOAT:
		return vk::ImageAspectFlagBits::eDepth;
	case RHIFormat::D24_UNORM_S8_UINT:
		return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	default:
		return vk::ImageAspectFlagBits::eColor;
	}
}

inline constexpr vk::DeviceSize getVkFormatByteSize(RHIFormat format) noexcept
{
	switch (format) {
	case RHIFormat::Unknown:
		return 0;
	case RHIFormat::R8_UINT:
	case RHIFormat::R8_SINT:
	case RHIFormat::R8_UNORM:
	case RHIFormat::R8_SNORM:
		return 1;
	case RHIFormat::RG8_UINT:
	case RHIFormat::RG8_SINT:
	case RHIFormat::RG8_UNORM:
	case RHIFormat::RG8_SNORM:
		return 2;
	case RHIFormat::RGBA8_UINT:
	case RHIFormat::RGBA8_SINT:
	case RHIFormat::RGBA8_UNORM:
	case RHIFormat::RGBA8_SRGB:
	case RHIFormat::RGBA8_SNORM:
	case RHIFormat::BGRA8_UNORM:
	case RHIFormat::BGRA8_SRGB:
		return 4;
	case RHIFormat::R16_UINT:
	case RHIFormat::R16_SINT:
	case RHIFormat::R16_UNORM:
	case RHIFormat::R16_SNORM:
	case RHIFormat::R16_FLOAT:
		return 2;
	case RHIFormat::RG16_UINT:
	case RHIFormat::RG16_SINT:
	case RHIFormat::RG16_UNORM:
	case RHIFormat::RG16_SNORM:
	case RHIFormat::RG16_FLOAT:
		return 4;
	case RHIFormat::RGBA16_UINT:
	case RHIFormat::RGBA16_SINT:
	case RHIFormat::RGBA16_FLOAT:
	case RHIFormat::RGBA16_UNORM:
	case RHIFormat::RGBA16_SNORM:
		return 8;
	case RHIFormat::R32_UINT:
	case RHIFormat::R32_SINT:
	case RHIFormat::R32_FLOAT:
		return 4;
	case RHIFormat::RG32_UINT:
	case RHIFormat::RG32_SINT:
	case RHIFormat::RG32_FLOAT:
		return 8;
	case RHIFormat::RGB32_UINT:
	case RHIFormat::RGB32_SINT:
	case RHIFormat::RGB32_FLOAT:
		return 12;
	case RHIFormat::RGBA32_UINT:
	case RHIFormat::RGBA32_SINT:
	case RHIFormat::RGBA32_FLOAT:
		return 16;
	case RHIFormat::D16_UNORM:
		return 2;
	case RHIFormat::D24_UNORM_S8_UINT:
		return 4;
	case RHIFormat::D32_FLOAT:
		return 4;
	}

	return 0;
}

inline constexpr VulkanTextureTransition getTextureTransition(RHIResourceState state) noexcept
{
	VulkanTextureTransition info{};

	if ((state & CopySource) != 0) {
		info.layout = vk::ImageLayout::eTransferSrcOptimal;
		info.stage = vk::PipelineStageFlagBits2::eCopy;
		info.access = vk::AccessFlagBits2::eTransferRead;
	} else if ((state & CopyDest) != 0) {
		info.layout = vk::ImageLayout::eTransferDstOptimal;
		info.stage = vk::PipelineStageFlagBits2::eCopy;
		info.access = vk::AccessFlagBits2::eTransferWrite;
	} else if ((state & RenderTarget) != 0) {
		info.layout = vk::ImageLayout::eColorAttachmentOptimal;
		info.stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		info.access = vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
	} else if ((state & DepthWrite) != 0) {
		info.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		info.stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
		info.access = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
	} else if ((state & DepthRead) != 0) {
		info.layout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
		info.stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests | vk::PipelineStageFlagBits2::eFragmentShader;
		info.access = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eShaderSampledRead;
	} else if ((state & UnorderedAccess) != 0) {
		info.layout = vk::ImageLayout::eGeneral;
		info.stage = vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eComputeShader;
		info.access = vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite;
	} else if ((state & ShaderResource) != 0) {
		info.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
		info.stage = vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eComputeShader;
		info.access = vk::AccessFlagBits2::eShaderSampledRead;
	} else if ((state & Present) != 0) {
		info.layout = vk::ImageLayout::ePresentSrcKHR;
		info.stage = vk::PipelineStageFlagBits2::eAllCommands;
		info.access = vk::AccessFlagBits2::eMemoryRead;
	} else {
		info.layout = vk::ImageLayout::eUndefined;
		info.stage = vk::PipelineStageFlagBits2::eTopOfPipe;
		info.access = {};
	}

	return info;
}

inline constexpr VulkanBufferTransition getBufferTransition(RHIResourceState state) noexcept
{
	VulkanBufferTransition info{};

	if (state == Unknown) {
		info.stage = vk::PipelineStageFlagBits2::eTopOfPipe;
		info.access = {};
	} else if ((state & CopySource) != 0) {
		info.stage = vk::PipelineStageFlagBits2::eCopy;
		info.access = vk::AccessFlagBits2::eTransferRead;
	} else if ((state & CopyDest) != 0) {
		info.stage = vk::PipelineStageFlagBits2::eCopy;
		info.access = vk::AccessFlagBits2::eTransferWrite;
	} else if ((state & VertexBuffer) != 0) {
		info.stage = vk::PipelineStageFlagBits2::eVertexAttributeInput;
		info.access = vk::AccessFlagBits2::eVertexAttributeRead;
	} else if ((state & IndexBuffer) != 0) {
		info.stage = vk::PipelineStageFlagBits2::eIndexInput;
		info.access = vk::AccessFlagBits2::eIndexRead;
	} else if ((state & ConstantBuffer) != 0) {
		info.stage = vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eComputeShader;
		info.access = vk::AccessFlagBits2::eUniformRead;
	} else if ((state & IndirectBuffer) != 0) {
		info.stage = vk::PipelineStageFlagBits2::eDrawIndirect;
		info.access = vk::AccessFlagBits2::eIndirectCommandRead;
	} else if ((state & UnorderedAccess) != 0) {
		info.stage = vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eComputeShader;
		info.access = vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite;
	} else if ((state & ShaderResource) != 0) {
		info.stage = vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eComputeShader;
		info.access = vk::AccessFlagBits2::eShaderRead;
	} else {
		info.stage = vk::PipelineStageFlagBits2::eAllCommands;
		info.access = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
	}

	return info;
}

}        // namespace Vortex

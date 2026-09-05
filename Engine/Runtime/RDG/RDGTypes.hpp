export module Runtime.RDG:Types;

import Core;
import Runtime.RHI;

export namespace Vortex {

struct RDGResource;
struct RDGTexture;
struct RDGBuffer;
struct RDGTextureView;
struct RDGBufferView;
struct RDGGraph;
class RDGPassBuilder;
class RDGPassContext;

using RDGTextureRef = RDGTexture*;
using RDGBufferRef = RDGBuffer*;
using RDGTextureViewRef = RDGTextureView*;
using RDGBufferViewRef = RDGBufferView*;

enum class RDGResourceType : uint8 {
	Texture,
	Buffer,
};

enum class RDGAccess : uint8 {
	Read,
	Write,
	ReadWrite,
};

enum class RDGPassType : uint8 {
	Raster,
	Compute,
	Copy,
};

enum class RDGPassFlags : uint8 {
	None = 0,
	NeverCull = 1 << 0,
};

struct RDGBarrier {
	RDGResource* resource{};
	RHIResourceState before{Unknown};
	RHIResourceState after{Unknown};
};

struct RDGResourceAccess {
	RDGResource* resource{};
	RDGAccess access{RDGAccess::Read};
	RHIResourceState state{Unknown};
};

struct RDGAttachment {
	RDGTextureRef texture{};
	RHILoadOp load_op{RHILoadOp::Load};
	RHIStoreOp store_op{RHIStoreOp::Store};
	RHIClearValue clear_value{};
	bool read_only{};
};

struct RDGRenderTargets {
	std::vector<RDGAttachment> colors{};
	std::optional<RDGAttachment> depth{};
};

struct RDGPassHandle {
	uint32 index{std::numeric_limits<uint32>::max()};

	bool operator==(const RDGPassHandle&) const noexcept = default;
	bool valid() const noexcept { return index != std::numeric_limits<uint32>::max(); }
};

struct RDGPassAccess {
	std::vector<RDGResourceAccess> accesses{};
	std::vector<RDGTextureViewRef> texture_views{};
	std::vector<RDGBufferViewRef> buffer_views{};
};

struct RDGPassState {
	std::vector<RDGPassHandle> dependencies{};
	std::vector<RDGBarrier> barriers{};
	bool culled{true};
};


template <typename Ref>
concept RDGAccessReference = std::is_convertible_v<Ref, RDGResource*> ||
    std::same_as<Ref, RDGTextureViewRef> || std::same_as<Ref, RDGBufferViewRef>;

template <>
struct EnableEnumFlags<RDGPassFlags> : std::true_type {};

constexpr bool reads(RDGAccess access) noexcept
{
	return access == RDGAccess::Read || access == RDGAccess::ReadWrite;
}

constexpr bool writes(RDGAccess access) noexcept
{
	return access == RDGAccess::Write || access == RDGAccess::ReadWrite;
}

constexpr bool hasAnyFlags(RDGPassFlags value, RDGPassFlags flags) noexcept
{
	return (value & flags) != RDGPassFlags::None;
}

}        // namespace Vortex

export module Runtime.RDG:Resource;

import Core;
import :Types;
import Runtime.RHI;

export namespace Vortex {

struct RDGResource {
	std::string name{};
	uint32 index{};

	RDGGraph* owner{};
	RDGResourceType type{RDGResourceType::Texture};

	bool external{};
	bool output{};

	std::optional<uint32> first_use{};
	std::optional<uint32> last_use{};

	RHIResourceState initial_state{Unknown};
	RHIResourceState final_state{Unknown};
};

struct RDGTexture final : RDGResource {
	RHITextureDesc desc{};
	RHIRef<RHITexture> texture{};
};

struct RDGBuffer final : RDGResource {
	RHIBufferDesc desc{};
	RHIRef<RHIBuffer> buffer{};
};

struct RDGTextureView final {
	std::string name{};
	uint32 index{};

	RDGTexture* texture{};
	RHITextureViewDesc desc{};
	RHIRef<RHITextureView> view{};
};

struct RDGBufferView final {
	std::string name{};
	uint32 index{};

	RDGBuffer* buffer{};
	RHIBufferViewDesc desc{};
	RHIRef<RHIBufferView> view{};
};

}        // namespace Vortex

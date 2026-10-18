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

struct RDGResourceStats {
	uint64 texture_count{};
	uint64 buffer_count{};
	uint64 texture_allocations{};
	uint64 buffer_allocations{};
	uint64 texture_reuses{};
	uint64 buffer_reuses{};
};


class RDGResourcePool {
private:
	template <typename T>
	struct Entry {
		RHIRef<T> resource{};
		uint32 idle_frames{};
	};

	std::vector<Entry<RHIBuffer>> buffers{};
	std::vector<Entry<RHITexture>> textures{};
	RDGResourceStats stats{};

	RHIDevice& device;

public:
	RDGResourcePool(RHIDevice& device) : device(device) {}

	RDGResourcePool(const RDGResourcePool&) = delete;
	RDGResourcePool& operator=(const RDGResourcePool&) = delete;

	RDGResourcePool(RDGResourcePool&&) = delete;
	RDGResourcePool& operator=(RDGResourcePool&&) = delete;

	RHIRef<RHIBuffer> acquireBuffer(const RHIBufferDesc& desc);
	RHIRef<RHITexture> acquireTexture(const RHITextureDesc& desc);

	void collect(uint32 max_idle_frames = 60);
	void clear() noexcept;

	RDGResourceStats getStats() const noexcept;
	RHIDevice& getDevice() const noexcept { return device; }
};

}        // namespace Vortex

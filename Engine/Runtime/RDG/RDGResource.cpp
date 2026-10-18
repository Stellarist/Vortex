module Runtime.RDG;

namespace Vortex {

RHIRef<RHIBuffer> RDGResourcePool::acquireBuffer(const RHIBufferDesc& desc)
{
	validateRHIBufferDesc(desc);
	for (auto& entry : buffers) {
		if (entry.resource->getRefCount() == 1 && entry.resource->getDesc() == desc) {
			entry.idle_frames = 0;
			++stats.buffer_reuses;
			return entry.resource;
		}
	}

	auto resource = device.createBuffer(desc);
	CHECK(resource, "Failed to allocate an RDG pooled buffer");

	buffers.push_back({resource});
	++stats.buffer_allocations;
	return resource;
}

RHIRef<RHITexture> RDGResourcePool::acquireTexture(const RHITextureDesc& desc)
{
	validateRHITextureDesc(desc);
	for (auto& entry : textures) {
		if (entry.resource->getRefCount() == 1 && entry.resource->getDesc() == desc) {
			entry.idle_frames = 0;
			++stats.texture_reuses;
			return entry.resource;
		}
	}

	auto resource = device.createTexture(desc);
	CHECK(resource, "Failed to allocate an RDG pooled texture");

	textures.push_back({resource});
	++stats.texture_allocations;
	return resource;
}

void RDGResourcePool::collect(uint32 max_idle_frames)
{
	auto collect_entries = [max_idle_frames](auto& entries) {
		for (auto& entry : entries) {
			if (entry.resource->getRefCount() != 1)
				entry.idle_frames = 0;
			else if (entry.idle_frames < max_idle_frames)
				++entry.idle_frames;
		}

		std::erase_if(entries, [max_idle_frames](const auto& entry) {
			return entry.resource->getRefCount() == 1 && entry.idle_frames >= max_idle_frames;
		});
	};

	collect_entries(textures);
	collect_entries(buffers);
}

void RDGResourcePool::clear() noexcept
{
	textures.clear();
	buffers.clear();
}

RDGResourceStats RDGResourcePool::getStats() const noexcept
{
	auto result = stats;
	result.texture_count = textures.size();
	result.buffer_count = buffers.size();
	return result;
}

}        // namespace Vortex

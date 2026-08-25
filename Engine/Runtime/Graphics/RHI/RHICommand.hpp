export module Runtime.Graphics:RHI.Command;

import Core;
import :RHI.Buffer;
import :RHI.Framebuffer;
import :RHI.Pipeline;
import :RHI.Texture;

export namespace Vortex {

struct RHIVertexBufferBinding {
	RHIRef<RHIBuffer> buffer{};
	uint32            slot{};
	uint32            offset{};

	RHIVertexBufferBinding& setBuffer(RHIBuffer* new_buffer) noexcept
	{
		buffer = new_buffer;
		return *this;
	}

	RHIVertexBufferBinding& setSlot(uint32 new_slot) noexcept
	{
		slot = new_slot;
		return *this;
	}

	RHIVertexBufferBinding& setOffset(uint32 new_offset) noexcept
	{
		offset = new_offset;
		return *this;
	}
};


struct RHIIndexBufferBinding {
	RHIRef<RHIBuffer> buffer{};
	RHIIndexType      index_type{RHIIndexType::Uint32};
	uint32            offset{};

	RHIIndexBufferBinding& setBuffer(RHIBuffer* new_buffer) noexcept
	{
		buffer = new_buffer;
		return *this;
	}

	RHIIndexBufferBinding& setIndexType(RHIIndexType new_index_type) noexcept
	{
		index_type = new_index_type;
		return *this;
	}

	RHIIndexBufferBinding& setOffset(uint32 new_offset) noexcept
	{
		offset = new_offset;
		return *this;
	}
};


struct RHIGraphicsState {
	RHIRef<RHIGraphicsPipeline> pipeline{};
	RHIRef<RHIFramebuffer>      framebuffer{};
	RHIViewportState            viewport_state{};

	std::vector<RHIRef<RHIBindingSet>>  binding_sets{};
	std::vector<RHIVertexBufferBinding> vertex_buffers{};
	RHIIndexBufferBinding               index_buffer{};

	RHIGraphicsState& setPipeline(RHIGraphicsPipeline* new_pipeline) noexcept
	{
		pipeline = new_pipeline;
		return *this;
	}

	RHIGraphicsState& setFramebuffer(RHIFramebuffer* new_framebuffer) noexcept
	{
		framebuffer = new_framebuffer;
		return *this;
	}

	RHIGraphicsState& setViewport(const RHIViewportState& new_viewport_state)
	{
		viewport_state = new_viewport_state;
		return *this;
	}

	RHIGraphicsState& addBindingSet(RHIBindingSet* new_binding_set)
	{
		binding_sets.emplace_back(new_binding_set);
		return *this;
	}

	RHIGraphicsState& addVertexBuffer(const RHIVertexBufferBinding& new_vert_buffer)
	{
		vertex_buffers.push_back(new_vert_buffer);
		return *this;
	}

	RHIGraphicsState& setIndexBuffer(const RHIIndexBufferBinding& new_index_buffer) noexcept
	{
		index_buffer = new_index_buffer;
		return *this;
	}
};


struct RHICommandListDesc {
	RHICommandQueue queue_type{RHICommandQueue::Graphics};
};

class RHICommandList : public RHIResource {
public:
	virtual void open() = 0;
	virtual void close() = 0;
	virtual void clear() = 0;

	virtual void clearTexture(RHITexture* texture, const RHIColor& clear_color) = 0;
	virtual void clearDepthTexture(RHITexture* texture, bool clear_depth, float depth, bool clear_stencil, uint8 stencil) = 0;
	virtual void copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice) = 0;
	virtual void copyTexture(RHIStagingTexture* dst_staging, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice) = 0;
	virtual void copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHIStagingTexture* src_staging, const RHITextureSlice& src_slice) = 0;
	virtual void writeTexture(RHITexture* texture, const RHITextureSlice& slice, const void* data, uint64 size) = 0;
	virtual void transitionTexture(RHITexture* texture, RHIResourceState new_state) = 0;

	virtual void clearBuffer(RHIBuffer* buffer, uint32 clear_value) = 0;
	virtual void copyBuffer(RHIBuffer* dst_buffer, uint64 dst_offset, RHIBuffer* src_buffer, uint64 src_offset, uint64 size) = 0;
	virtual void writeBuffer(RHIBuffer* buffer, uint64 offset, const void* data, uint64 size) = 0;
	virtual void transitionBuffer(RHIBuffer* buffer, RHIResourceState new_state) = 0;

	virtual void setPushConstants(const void* data, size_t size) = 0;
	virtual void draw(const RHIDrawArguments& args) = 0;
	virtual void drawIndexed(const RHIDrawArguments& args) = 0;

	virtual void setGraphicsState(const RHIGraphicsState& state) = 0;
};

}        // namespace Vortex

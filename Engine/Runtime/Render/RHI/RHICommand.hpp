#pragma once

#include "RHIPipeline.hpp"

// vertex buffer binding
struct RHIVertexBufferBinding {
	RHIBuffer* buffer{};
	uint32_t   slot{};
	uint32_t   offset{};

	RHIVertexBufferBinding& setBuffer(RHIBuffer* new_buffer)
	{
		buffer = new_buffer;
		return *this;
	}

	RHIVertexBufferBinding& setSlot(uint32_t new_slot)
	{
		slot = new_slot;
		return *this;
	}

	RHIVertexBufferBinding& setOffset(uint32_t new_offset)
	{
		offset = new_offset;
		return *this;
	}
};


// index buffer binding
struct RHIIndexBufferBinding {
	RHIBuffer*   buffer{};
	RHIIndexType index_type{RHIIndexType::Uint32};
	uint32_t     offset{};

	RHIIndexBufferBinding& setBuffer(RHIBuffer* new_buffer)
	{
		buffer = new_buffer;
		return *this;
	}

	RHIIndexBufferBinding& setIndexType(RHIIndexType new_index_type)
	{
		index_type = new_index_type;
		return *this;
	}

	RHIIndexBufferBinding& setOffset(uint32_t new_offset)
	{
		offset = new_offset;
		return *this;
	}
};


// graphics state
struct RHIGraphicsState {
	RHIGraphicsPipeline* pipeline{};
	RHIFrameBuffer*      framebuffer{};
	RHIViewportState     viewport_state{};

	std::vector<RHIDescriptorSet*>      binding_sets{};
	std::vector<RHIVertexBufferBinding> vertex_buffers{};
	RHIIndexBufferBinding               index_buffer{};

	RHIGraphicsState& setPipeline(RHIGraphicsPipeline* new_pipeline)
	{
		pipeline = new_pipeline;
		return *this;
	}

	RHIGraphicsState& setFrameBuffer(RHIFrameBuffer* new_framebuffer)
	{
		framebuffer = new_framebuffer;
		return *this;
	}

	RHIGraphicsState& setViewport(const RHIViewportState& new_viewport_state)
	{
		viewport_state = new_viewport_state;
		return *this;
	}

	RHIGraphicsState& addBindingSet(RHIDescriptorSet* new_binding_set)
	{
		binding_sets.push_back(new_binding_set);
		return *this;
	}

	RHIGraphicsState& addVertexBuffer(const RHIVertexBufferBinding& new_vert_buffer)
	{
		vertex_buffers.push_back(new_vert_buffer);
		return *this;
	}

	RHIGraphicsState& setIndexBuffer(const RHIIndexBufferBinding& new_index_buffer)
	{
		index_buffer = new_index_buffer;
		return *this;
	}
};


// command list
struct RHICommandListDesc {
	RHICommandQueue queue_type{RHICommandQueue::Graphics};
};

class RHICommandList : public RHIResource {
public:
	virtual void open() = 0;
	virtual void close() = 0;
	virtual void clear() = 0;

	virtual void clearTexture(RHITexture* texture, const RHIColor& clear_color) = 0;
	virtual void clearDepthTexture(RHITexture* texture, bool clear_depth, float depth, bool clear_stencil, uint8_t stencil) = 0;
	virtual void copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice) = 0;
	virtual void copyTexture(RHIStagingTexture* dst_staging, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice) = 0;
	virtual void copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHIStagingTexture* src_staging, const RHITextureSlice& src_slice) = 0;
	virtual void writeTexture(RHITexture* texture, const RHITextureSlice& slice, const void* data, uint64_t size) = 0;
	virtual void transitionTexture(RHITexture* texture, RHIResourceState new_state) = 0;

	virtual void clearBuffer(RHIBuffer* buffer, uint32_t clear_value) = 0;
	virtual void copyBuffer(RHIBuffer* dst_buffer, uint64_t dst_offset, RHIBuffer* src_buffer, uint64_t src_offset, uint64_t size) = 0;
	virtual void writeBuffer(RHIBuffer* buffer, uint64_t offset, const void* data, uint64_t size) = 0;
	virtual void transitionBuffer(RHIBuffer* buffer, RHIResourceState new_state) = 0;

	virtual void pushConstants(const void* data, size_t size) = 0;
	virtual void draw(const RHIDrawArguments& args) = 0;
	virtual void drawIndexed(const RHIDrawArguments& args) = 0;

	virtual void setGraphicsState(const RHIGraphicsState& state) = 0;
};

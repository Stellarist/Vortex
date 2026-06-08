#pragma once

#include <deque>

#include "VulkanDevice.hpp"
#include "Runtime/Render/RHI/RHICommand.hpp"

class VulkanCommandPool;

// command buffer
class VulkanCommandBuffer {
private:
	vk::CommandBuffer buffer{};

	std::vector<std::shared_ptr<RHIResource>> tracked_resources{};

	VulkanDevice&      device;
	VulkanCommandPool& pool;

public:
	VulkanCommandBuffer(VulkanDevice& device, VulkanCommandPool& pool);
	~VulkanCommandBuffer();

	void allocMemory();
	void freeMemory();

	void beginRendering(const vk::RenderingInfo& render_info);
	void endRendering();

	void trackResource(std::shared_ptr<RHIResource> resource);
	void resetResources();

	vk::CommandBuffer getHandle() const { return buffer; }

	VulkanCommandPool& getPool() const { return pool; }
};


// command pool
class VulkanCommandPool {
private:
	vk::CommandPool pool{};

	std::deque<VulkanCommandBuffer*> cmd_buffers{};
	std::deque<VulkanCommandBuffer*> free_cmd_buffers{};

	VulkanDevice& device;
	VulkanQueue&  queue;

public:
	VulkanCommandPool(VulkanDevice& device, VulkanQueue& queue);
	~VulkanCommandPool();

	VulkanCommandBuffer* createCommandBuffer();

	void releaseCommandBuffer(VulkanCommandBuffer* cmd_buffer);

	vk::CommandPool getHandle() const { return pool; }

	VulkanQueue& getQueue() const { return queue; }
};


// command list
class VulkanCommandList : public RHICommandList {
private:
	RHICommandListDesc desc{};

	RHIGraphicsState graphics_state{};

	std::shared_ptr<VulkanCommandBuffer> current_command{};

	vk::PipelineLayout   current_layout{};
	vk::ShaderStageFlags current_visibility{};

	bool rendering{};

	VulkanDevice& device;

	void beginRenderPass(RHIFrameBuffer& framebuffer);
	void endRenderPass();

public:
	VulkanCommandList(VulkanDevice& device, RHICommandListDesc desc) : desc(std::move(desc)), device(device) {}
	~VulkanCommandList() override = default;

	const RHICommandListDesc& getDesc() const { return desc; }

	std::shared_ptr<VulkanCommandBuffer> getCurrentCommand() const { return current_command; }

	void open() override;
	void close() override;
	void clear() override;

	void clearTexture(RHITexture* texture, const RHIColor& clear_color) override;
	void clearDepthTexture(RHITexture* texture, bool clear_depth, float depth, bool clear_stencil, uint8_t stencil) override;
	void copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice) override;
	void copyTexture(RHIStagingTexture* dst_staging, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice) override;
	void copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHIStagingTexture* src_staging, const RHITextureSlice& src_slice) override;
	void writeTexture(RHITexture* texture, const RHITextureSlice& slice, const void* data, uint64_t size) override;
	void transitionTexture(RHITexture* texture, RHIResourceState new_state) override;

	void clearBuffer(RHIBuffer* buffer, uint32_t clear_value) override;
	void copyBuffer(RHIBuffer* dst_buffer, uint64_t dst_offset, RHIBuffer* src_buffer, uint64_t src_offset, uint64_t size) override;
	void writeBuffer(RHIBuffer* buffer, uint64_t offset, const void* data, uint64_t size) override;
	void transitionBuffer(RHIBuffer* buffer, RHIResourceState new_state) override;

	void pushConstants(const void* data, size_t size) override;
	void draw(const RHIDrawArguments& args) override;
	void drawIndexed(const RHIDrawArguments& args) override;

	void setGraphicsState(const RHIGraphicsState& state) override;
};

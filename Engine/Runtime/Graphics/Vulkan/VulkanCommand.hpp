export module Runtime.Graphics:Vulkan.Command;

import vulkan;
import Core;
import :Vulkan.Device;
import :RHI.Command;

export namespace Vortex {

class VulkanCommandPool;

// command buffer
class VulkanCommandBuffer {
private:
	vk::CommandBuffer buffer{};

	std::vector<RHIRef<RHIResource>> tracked_resources{};

	VulkanDevice& device;
	VulkanCommandPool& pool;

public:
	VulkanCommandBuffer(VulkanDevice& device, VulkanCommandPool& pool);
	~VulkanCommandBuffer();

	void allocMemory();
	void freeMemory();

	void beginRendering(const vk::RenderingInfo& render_info);
	void endRendering();

	void trackResource(RHIResource* resource);
	void resetResources();

	vk::CommandBuffer getHandle() const noexcept { return buffer; }

	VulkanCommandPool& getPool() const noexcept { return pool; }
};


// command pool
class VulkanCommandPool {
private:
	vk::CommandPool pool{};

	std::deque<VulkanCommandBuffer*> cmd_buffers{};
	std::deque<VulkanCommandBuffer*> free_cmd_buffers{};

	VulkanDevice& device;
	VulkanQueue& queue;

public:
	VulkanCommandPool(VulkanDevice& device, VulkanQueue& queue);
	~VulkanCommandPool();

	VulkanCommandBuffer* createCommandBuffer();

	void releaseCommandBuffer(VulkanCommandBuffer* cmd_buffer);

	vk::CommandPool getHandle() const noexcept { return pool; }

	VulkanQueue& getQueue() const noexcept { return queue; }
};


// command list
class VulkanCommandList : public RHICommandList {
private:
	RHICommandListDesc desc{};

	RHIGraphicsState graphics_state{};
	RHIComputeState compute_state{};

	std::shared_ptr<VulkanCommandBuffer> current_command{};

	vk::PipelineLayout current_layout{};
	RHIShaderType current_push_constant_visibility{};

	bool rendering{};

	VulkanDevice& device;

	void beginRenderPass(RHIFramebuffer& framebuffer);
	void endRenderPass();

public:
	VulkanCommandList(VulkanDevice& device, RHICommandListDesc desc) :
	    device(device), desc(std::move(desc)) {}
	~VulkanCommandList() override = default;

	const RHICommandListDesc& getDesc() const noexcept { return desc; }

	std::shared_ptr<VulkanCommandBuffer> getCurrentCommand() const noexcept { return current_command; }

	void open() override;
	void close() override;
	void clear() override;

	void clearTexture(RHITexture* texture, const RHIColor& clear_color) override;
	void clearDepthTexture(RHITexture* texture, bool clear_depth, float depth, bool clear_stencil, uint8 stencil) override;
	void copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice) override;
	void copyTexture(RHIStagingTexture* dst_staging, const RHITextureSlice& dst_slice, RHITexture* src_texture, const RHITextureSlice& src_slice) override;
	void copyTexture(RHITexture* dst_texture, const RHITextureSlice& dst_slice, RHIStagingTexture* src_staging, const RHITextureSlice& src_slice) override;
	void writeTexture(RHITexture* texture, const RHITextureSlice& slice, const void* data, uint64 size) override;
	void transitionTexture(RHITexture* texture, RHIResourceState before, RHIResourceState after) override;

	void clearBuffer(RHIBuffer* buffer, uint32 clear_value) override;
	void copyBuffer(RHIBuffer* dst_buffer, uint64 dst_offset, RHIBuffer* src_buffer, uint64 src_offset, uint64 size) override;
	void writeBuffer(RHIBuffer* buffer, uint64 offset, const void* data, uint64 size) override;
	void transitionBuffer(RHIBuffer* buffer, RHIResourceState before, RHIResourceState after) override;

	void setPushConstants(const void* data, size_t size) override;
	void draw(const RHIDrawArguments& args) override;
	void drawIndexed(const RHIDrawArguments& args) override;
	void dispatch(uint32 group_count_x, uint32 group_count_y, uint32 group_count_z) override;

	void beginRendering(RHIFramebuffer* framebuffer) override;
	void endRendering() override;
	void setGraphicsState(const RHIGraphicsState& state) override;
	void setComputeState(const RHIComputeState& state) override;

	void beginDebugLabel(std::string_view name) override;
	void endDebugLabel() override;
};

}        // namespace Vortex

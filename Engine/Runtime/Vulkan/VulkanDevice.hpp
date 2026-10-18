module;

#include <vk_mem_alloc.h>

export module Runtime.Vulkan:Device;

import vulkan;
import Core;
import Runtime.RHI;

export namespace Vortex {

class VulkanContext;
class VulkanQueue;
class VulkanCommandList;
class VulkanBuffer;
class VulkanBufferView;
class VulkanTexture;
class VulkanTextureView;
class VulkanStagingTexture;
class VulkanSampler;
class VulkanShader;
class VulkanBindingLayout;
class VulkanBindingSet;
class VulkanGraphicsPipeline;
class VulkanComputePipeline;

class VulkanDevice : public RHIDevice {
private:
	vk::Device device{};

	VmaAllocator allocator{};

	std::unique_ptr<VulkanQueue> queue{};

	PFN_vkSetDebugUtilsObjectNameEXT set_debug_name{};
	PFN_vkCmdBeginDebugUtilsLabelEXT begin_debug_label{};
	PFN_vkCmdEndDebugUtilsLabelEXT end_debug_label{};

	VulkanContext* context{};

public:
	VulkanDevice(VulkanContext& context, vk::Device device);
	~VulkanDevice() noexcept override;

	RHIAPI getAPI() const noexcept override { return RHIAPI::Vulkan; }

	template <typename Handle>
	void setName(Handle handle, const std::string& name) const noexcept;

	RHIRef<RHIBuffer> createBuffer(const RHIBufferDesc& desc) override;
	RHIRef<RHIBufferView> createBufferView(const RHIBufferViewDesc& desc) override;
	RHIRef<RHITexture> createTexture(const RHITextureDesc& desc) override;
	RHIRef<RHITextureView> createTextureView(const RHITextureViewDesc& desc) override;
	RHIRef<RHIStagingTexture> createStagingTexture(const RHITextureDesc& desc, RHIAccessMode access) override;
	RHIRef<RHISampler> createSampler(const RHISamplerDesc& desc) override;
	RHIRef<RHIShader> createShader(const RHIShaderDesc& desc, std::span<const std::byte> bytecode) override;
	RHIRef<RHIFramebuffer> createFramebuffer(const RHIFramebufferDesc& desc) override;

	RHIRef<RHIInputLayout> createInputLayout(const RHIInputLayoutDesc& desc) override;
	RHIRef<RHIBindingLayout> createBindingLayout(const RHIBindingLayoutDesc& desc) override;
	RHIRef<RHIBindingLayout> createBindlessLayout(const RHIBindlessLayoutDesc& desc) override;
	RHIRef<RHIBindingSet> createBindingSet(const RHIBindingSetDesc& desc, const RHIBindingLayout& layout) override;
	RHIRef<RHIDescriptorTable> createDescriptorTable(const RHIBindingLayout& layout, uint32 capacity) override;
	RHIRef<RHICommandList> createCommandList(const RHICommandListDesc& desc) override;
	RHIRef<RHIGraphicsPipeline> createGraphicsPipeline(const RHIGraphicsPipelineDesc& desc) override;
	RHIRef<RHIComputePipeline> createComputePipeline(const RHIComputePipelineDesc& desc) override;

	void* mapBuffer(RHIBuffer* buffer, RHIAccessMode mode) const override;
	void unmapBuffer(RHIBuffer* buffer) const noexcept override;
	void* mapStagingTexture(RHIStagingTexture* staging_texture, RHIAccessMode mode,
	    uint64& row_pitch, uint64& depth_pitch, uint32 mip = 0, uint32 layer = 0) const override;
	void unmapStagingTexture(RHIStagingTexture* staging_texture) const noexcept override;

	void executeCommandList(RHICommandList* command_list) override;

	void destroyBuffer(VulkanBuffer* buffer) noexcept;
	void destroyBufferView(VulkanBufferView* view) noexcept;
	void destroyTexture(VulkanTexture* texture) noexcept;
	void destroyTextureView(VulkanTextureView* view) noexcept;
	void destroyStagingTexture(VulkanStagingTexture* staging_texture) noexcept;
	void destroySampler(VulkanSampler* sampler) noexcept;

	void destroyBindingLayout(VulkanBindingLayout* layout) noexcept;
	void destroyBindingSet(VulkanBindingSet* set) noexcept;
	void destroyGraphicsPipeline(VulkanGraphicsPipeline* pipeline) noexcept;
	void destroyComputePipeline(VulkanComputePipeline* pipeline) noexcept;

	vk::Device getHandle() const noexcept { return device; }

	void beginDebugLabel(vk::CommandBuffer command, std::string_view name) const;
	void endDebugLabel(vk::CommandBuffer command) const noexcept;

	VulkanContext& getContext() const noexcept { return *context; }
	VulkanQueue& getQueue() const noexcept { return *queue; }

	void waitIdle() override;
};

template <typename Handle>
inline void VulkanDevice::setName(Handle handle, const std::string& name) const noexcept
{
	if (!handle || name.empty())
		return;
	typename Handle::CType native_handle = handle;

	uint64 object_handle{};
	static_assert(sizeof(native_handle) <= sizeof(object_handle));
	std::memcpy(&object_handle, &native_handle, sizeof(native_handle));
	if (!set_debug_name)
		return;

	VkDebugUtilsObjectNameInfoEXT info{
	    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
	    .objectType = static_cast<VkObjectType>(Handle::objectType),
	    .objectHandle = object_handle,
	    .pObjectName = name.c_str(),
	};

	static_cast<void>(set_debug_name(static_cast<VkDevice>(device), &info));
}

}        // namespace Vortex

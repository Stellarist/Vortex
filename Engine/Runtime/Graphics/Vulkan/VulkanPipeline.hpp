module;

#include <cassert>

export module Runtime.Graphics:Vulkan.Pipeline;

import vulkan;
import Core;
import :Vulkan.Device;

export namespace Vortex {

// Input Layout
class VulkanInputLayout : public RHIInputLayout {
private:
	RHIInputLayoutDesc desc{};

	std::vector<vk::VertexInputBindingDescription>   bindings{};
	std::vector<vk::VertexInputAttributeDescription> attributes{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanInputLayout(VulkanDevice& device, RHIInputLayoutDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanInputLayout() override = default;

	const RHIVertexAttributeDesc& getAttributeDesc(uint32 index) const noexcept override
	{
		assert(index < desc.attribute_descs.size() && "Vertex attribute index is outside the input layout.");
		return desc.attribute_descs[index];
	}

	uint32 getAttributeCount() const noexcept override { return static_cast<uint32>(desc.attribute_descs.size()); }

	const std::vector<vk::VertexInputBindingDescription>&   getBindings() const noexcept { return bindings; }
	const std::vector<vk::VertexInputAttributeDescription>& getAttributes() const noexcept { return attributes; }
};


// Graphics Pipeline
class VulkanGraphicsPipeline : public RHIGraphicsPipeline {
private:
	RHIGraphicsPipelineDesc desc{};

	RHIShaderType shader_mask{};
	RHIShaderType push_constant_visibility{};
	uint32        push_constant_size{};

	vk::Pipeline       pipeline{};
	vk::PipelineLayout layout{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanGraphicsPipeline(VulkanDevice& device, RHIGraphicsPipelineDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanGraphicsPipeline() override { device.destroyGraphicsPipeline(this); }

	const RHIGraphicsPipelineDesc& getDesc() const noexcept override { return desc; }

	RHIShaderType getShaderMask() const noexcept { return shader_mask; }
	RHIShaderType getPushConstantVisibility() const noexcept { return push_constant_visibility; }
	uint32        getPushConstantSize() const noexcept { return push_constant_size; }

	vk::Pipeline       getHandle() const noexcept { return pipeline; }
	vk::PipelineLayout getLayout() const noexcept { return layout; }
};

}        // namespace Vortex

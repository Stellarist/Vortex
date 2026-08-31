module;

#include <cassert>

export module Runtime.Graphics:Vulkan.Pipeline;

import vulkan;
import Core;
import :Vulkan.Device;

export namespace Vortex {

class VulkanInputLayout : public RHIInputLayout {
private:
	RHIInputLayoutDesc desc{};

	std::vector<vk::VertexInputBindingDescription>   bindings{};
	std::vector<vk::VertexInputAttributeDescription> attributes{};

	friend class VulkanDevice;

public:
	VulkanInputLayout(RHIInputLayoutDesc desc) : desc(std::move(desc)) {}
	~VulkanInputLayout() override = default;

	const RHIVertexAttributeDesc& getAttributeDesc(uint32 index) const override
	{
		CHECK(Range, index < desc.attribute_descs.size(),
		    "Vertex attribute index {} is outside the input layout", index);
		return desc.attribute_descs[index];
	}

	uint32 getAttributeCount() const noexcept override { return static_cast<uint32>(desc.attribute_descs.size()); }

	const std::vector<vk::VertexInputBindingDescription>& getBindings() const noexcept { return bindings; }
	const std::vector<vk::VertexInputAttributeDescription>& getAttributes() const noexcept { return attributes; }
};


class VulkanGraphicsPipeline : public RHIGraphicsPipeline {
private:
	RHIGraphicsPipelineDesc desc{};

	RHIShaderType push_constant_visibility{};

	vk::Pipeline       pipeline{};
	vk::PipelineLayout layout{};

	VulkanDevice& device;

	friend class VulkanDevice;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(pipeline, name);
		device.setName(layout, name);
	}

public:
	VulkanGraphicsPipeline(VulkanDevice& device, RHIGraphicsPipelineDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanGraphicsPipeline() override { device.destroyGraphicsPipeline(this); }

	const RHIGraphicsPipelineDesc& getDesc() const noexcept override { return desc; }

	RHIShaderType getPushConstantVisibility() const noexcept { return push_constant_visibility; }

	vk::Pipeline getHandle() const noexcept { return pipeline; }
	vk::PipelineLayout getLayout() const noexcept { return layout; }
};


class VulkanComputePipeline : public RHIComputePipeline {
private:
	RHIComputePipelineDesc desc{};

	RHIShaderType push_constant_visibility{};

	vk::Pipeline       pipeline{};
	vk::PipelineLayout layout{};

	VulkanDevice& device;

	friend class VulkanDevice;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(pipeline, name);
		device.setName(layout, name);
	}

public:
	VulkanComputePipeline(VulkanDevice& device, RHIComputePipelineDesc desc) : desc(std::move(desc)), device(device) {}
	~VulkanComputePipeline() override { device.destroyComputePipeline(this); }

	const RHIComputePipelineDesc& getDesc() const noexcept override { return desc; }

	RHIShaderType getPushConstantVisibility() const noexcept { return push_constant_visibility; }
	vk::Pipeline getHandle() const noexcept { return pipeline; }
	vk::PipelineLayout getLayout() const noexcept { return layout; }
};

}        // namespace Vortex

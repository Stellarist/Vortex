#pragma once

#include "VulkanDevice.hpp"

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

	const RHIVertexAttributeDesc& getAttributeDesc(uint32_t index) const override { return desc.attribute_descs.at(index); }

	uint32_t getAttributeCount() const override { return static_cast<uint32_t>(desc.attribute_descs.size()); }

	const std::vector<vk::VertexInputBindingDescription>&   getBindings() const { return bindings; }
	const std::vector<vk::VertexInputAttributeDescription>& getAttributes() const { return attributes; }
};


// Graphics Pipeline
class VulkanGraphicsPipeline : public RHIGraphicsPipeline {
private:
	RHIGraphicsPipelineDesc desc{};

	RHIShaderType shader_mask{};

	vk::Pipeline       pipeline{};
	vk::PipelineLayout layout{};

	VulkanDevice& device;

	friend class VulkanDevice;

public:
	VulkanGraphicsPipeline(VulkanDevice& device, RHIGraphicsPipelineDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanGraphicsPipeline() override { device.destroyGraphicsPipeline(this); }

	const RHIGraphicsPipelineDesc& getDesc() const override { return desc; }

	RHIShaderType getShaderMask() const { return shader_mask; }

	vk::Pipeline       getHandle() const { return pipeline; }
	vk::PipelineLayout getLayout() const { return layout; }
};

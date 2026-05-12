#pragma once

#include "BasePath.hpp"
#include "Runtime/Render/Passes/GeometryPass.hpp"
#include "Runtime/Render/Passes/LightingPass.hpp"
#include "Runtime/Render/Backend/VulkanGraphicsPipeline.hpp"
#include "Runtime/Render/Backend/VulkanGBuffer.hpp"

class DeferredPath : public BasePath {
private:
	std::unique_ptr<GeometryPass> geometry_pass;
	std::unique_ptr<LightingPass> lighting_pass;

	std::unique_ptr<VulkanGraphicsPipeline> geometry_pipeline;
	std::unique_ptr<VulkanGraphicsPipeline> lighting_pipeline;

	std::unique_ptr<VulkanGBuffer> gbuffer;

	static std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;

	VulkanGraphicsPipelineConfig createGeometryPipelineConfig();
	VulkanGraphicsPipelineConfig createLightingPipelineConfig();

public:
	DeferredPath();
	~DeferredPath() override;

	void initialize(VulkanContext& context) override;
	void cleanup() override;
	void resize(uint32_t width, uint32_t height) override;

	void beginGeometryPass(vk::CommandBuffer command, vk::Extent2D extent);
	void endGeometryPass(vk::CommandBuffer command);

	void beginLightingPass(vk::CommandBuffer command, uint32_t image_index, vk::Extent2D extent);
	void endLightingPass(vk::CommandBuffer command);

	void bindDescriptor(vk::CommandBuffer command) const;

	DeferredPath& build(std::span<const vk::DescriptorSetLayout> geometry_layouts, std::span<const vk::PipelineShaderStageCreateInfo> geometry_stages,
	    std::span<const vk::DescriptorSetLayout> lighting_layouts, std::span<const vk::PipelineShaderStageCreateInfo> lighting_stages);

	GeometryPass& getGeometryPass() const;
	LightingPass& getLightingPass() const;

	VulkanGraphicsPipeline& getGeometryPipeline() const;
	VulkanGraphicsPipeline& getLightingPipeline() const;

	VulkanGBuffer& getGBuffer() const;
};

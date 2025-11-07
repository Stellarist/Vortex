#pragma once

#include "BasePath.hpp"
#include "Render/Passes/GeometryPass.hpp"
#include "Render/Passes/LightingPass.hpp"
#include "Render/Graphics/GraphicsPipeline.hpp"
#include "Render/Graphics/GBuffer.hpp"

class DeferredPath : public BasePath {
private:
	std::unique_ptr<GeometryPass> geometry_pass;
	std::unique_ptr<LightingPass> lighting_pass;

	std::unique_ptr<GraphicsPipeline> geometry_pipeline;
	std::unique_ptr<GraphicsPipeline> lighting_pipeline;

	std::unique_ptr<GBuffer> gbuffer;

	static std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;

	GraphicsPipelineConfig createGeometryPipelineConfig();
	GraphicsPipelineConfig createLightingPipelineConfig();

public:
	DeferredPath();
	~DeferredPath() override;

	void initialize(Context& context) override;
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

	GraphicsPipeline& getGeometryPipeline() const;
	GraphicsPipeline& getLightingPipeline() const;

	GBuffer& getGBuffer() const;
};

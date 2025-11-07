#pragma once

#include "BasePath.hpp"
#include "Render/Graphics/GraphicsPipeline.hpp"
#include "Render/Passes/ForwardPass.hpp"

class ForwardPath : public BasePath {
private:
	std::unique_ptr<ForwardPass> forward_pass;

	std::unique_ptr<GraphicsPipeline> forward_pipeline;

	GraphicsPipelineConfig createPipelineConfig();

public:
	ForwardPath();
	~ForwardPath() override;

	void initialize(Context& context) override;
	void cleanup() override;
	void resize(uint32_t width, uint32_t height) override;

	void beginForwardPass(vk::CommandBuffer command, uint32_t image_index, vk::Extent2D extent);
	void endForwardPass(vk::CommandBuffer command);

	ForwardPath& build(std::span<const vk::DescriptorSetLayout> forward_layouts, std::span<const vk::PipelineShaderStageCreateInfo> forward_stages);

	ForwardPass&      getForwardPass() const;
	GraphicsPipeline& getForwardPipeline() const;
};

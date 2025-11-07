#pragma once

#include "BasePass.hpp"
#include "Render/Graphics/Descriptor.hpp"
#include "Render/Graphics/GBuffer.hpp"

class LightingPass : public BasePass {
private:
	DescriptorSet                        gbuffer_descriptor;
	std::unique_ptr<DescriptorSetLayout> gbuffer_layout;
	std::unique_ptr<DescriptorPool>      gbuffer_pool;

	RenderPassConfig createConfig();

	void createFramebuffers();
	void createGBufferDescriptorSetLayout();
	void createGBufferDescriptorSet(const GBuffer& gbuffer);

public:
	LightingPass();
	~LightingPass() override;

	void initialize(Context& context, vk::Extent2D extent) override;
	void cleanup() override;
	void resize(vk::Extent2D new_extent) override;

	void setupGBuffer(const GBuffer& gbuffer);
	void updateGBufferDescriptorSet(const GBuffer& gbuffer);

	void bindGBufferDescriptor(vk::CommandBuffer command, vk::PipelineLayout pipeline_layout) const;

	const DescriptorSetLayout& getGBufferLayout() const;
};

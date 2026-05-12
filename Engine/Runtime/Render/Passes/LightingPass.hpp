#pragma once

#include "BasePass.hpp"
#include "Runtime/Render/Backend/VulkanDescriptor.hpp"
#include "Runtime/Render/Backend/VulkanGBuffer.hpp"

class LightingPass : public BasePass {
private:
	VulkanDescriptorSet                        gbuffer_descriptor;
	std::unique_ptr<VulkanDescriptorSetLayout> gbuffer_layout;
	std::unique_ptr<DescriptorPool>      gbuffer_pool;

	VulkanRenderPassConfig createConfig();

	void createFramebuffers();
	void createGBufferDescriptorSetLayout();
	void createGBufferDescriptorSet(const VulkanGBuffer& gbuffer);

public:
	LightingPass();
	~LightingPass() override;

	void initialize(VulkanContext& context, vk::Extent2D extent) override;
	void cleanup() override;
	void resize(vk::Extent2D new_extent) override;

	void setupGBuffer(const VulkanGBuffer& gbuffer);
	void updateGBufferDescriptorSet(const VulkanGBuffer& gbuffer);

	void bindGBufferDescriptor(vk::CommandBuffer command, vk::PipelineLayout pipeline_layout) const;

	const VulkanDescriptorSetLayout& getGBufferLayout() const;
};

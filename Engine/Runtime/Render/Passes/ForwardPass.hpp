#pragma once

#include "BasePass.hpp"
#include "Runtime/Render/Backend/VulkanImage.hpp"

class ForwardPass : public BasePass {
private:
	std::unique_ptr<VulkanImage> depth_image;

	VulkanRenderPassConfig createConfig();

	void createDepthImage();
	void createFramebuffers();

public:
	ForwardPass();
	~ForwardPass() override;

	void initialize(VulkanContext& context, vk::Extent2D extent) override;
	void cleanup() override;
	void resize(vk::Extent2D new_extent) override;

	VulkanImage& getDepthImage() const;
};

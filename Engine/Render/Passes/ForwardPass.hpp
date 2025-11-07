#pragma once

#include "BasePass.hpp"
#include "Render/Graphics/Image.hpp"

class ForwardPass : public BasePass {
private:
	std::unique_ptr<Image> depth_image;

	RenderPassConfig createConfig();

	void createDepthImage();
	void createFramebuffers();

public:
	ForwardPass();
	~ForwardPass() override;

	void initialize(Context& context, vk::Extent2D extent) override;
	void cleanup() override;
	void resize(vk::Extent2D new_extent) override;

	Image& getDepthImage() const;
};

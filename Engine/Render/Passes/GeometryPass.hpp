#pragma once

#include "BasePass.hpp"
#include "Render/Graphics/GBuffer.hpp"

class GeometryPass : public BasePass {
private:
	GBuffer* gbuffer{};

	std::unordered_map<GBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>> attachment_infos;

	RenderPassConfig createConfig();

	void createFramebuffers();
	void createAttachmentInfos();

public:
	GeometryPass();
	~GeometryPass() override;

	void initialize(Context& context, vk::Extent2D extent) override;
	void cleanup() override;
	void resize(vk::Extent2D new_extent) override;

	void setGBuffer(GBuffer& buffer);

	const std::unordered_map<GBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>>& getGBufferAttachmentInfos() const;
};

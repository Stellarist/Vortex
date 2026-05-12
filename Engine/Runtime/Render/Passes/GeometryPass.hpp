#pragma once

#include "BasePass.hpp"
#include "Runtime/Render/Backend/VulkanGBuffer.hpp"

class GeometryPass : public BasePass {
private:
	VulkanGBuffer* gbuffer{};

	std::unordered_map<VulkanGBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>> attachment_infos;

	VulkanRenderPassConfig createConfig();

	void createFramebuffers();
	void createAttachmentInfos();

public:
	GeometryPass();
	~GeometryPass() override;

	void initialize(VulkanContext& context, vk::Extent2D extent) override;
	void cleanup() override;
	void resize(vk::Extent2D new_extent) override;

	void setGBuffer(VulkanGBuffer& buffer);

	const std::unordered_map<VulkanGBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>>& getGBufferAttachmentInfos() const;
};

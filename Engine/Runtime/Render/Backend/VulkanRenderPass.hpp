#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"

struct VulkanRenderPassConfig {
	std::vector<vk::AttachmentDescription> attachments;
	std::vector<vk::SubpassDescription>    subpasses;
	std::vector<vk::SubpassDependency>     dependencies;
};

class VulkanRenderPass {
private:
	vk::RenderPass render_pass;

	std::vector<vk::Framebuffer> framebuffers;

	VulkanContext* context{};

	void create(const VulkanRenderPassConfig& config);

public:
	VulkanRenderPass(VulkanContext& context, const VulkanRenderPassConfig& config);
	~VulkanRenderPass();

	VulkanRenderPass(const VulkanRenderPass&) = delete;
	VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

	VulkanRenderPass(VulkanRenderPass&&) noexcept = default;
	VulkanRenderPass& operator=(VulkanRenderPass&&) noexcept = default;

	void createFramebuffers(std::span<const std::vector<vk::ImageView>> attachments_per_frame, vk::Extent2D extent);

	void begin(vk::CommandBuffer command_buffer, uint32_t framebuffer_index,
	    const vk::Extent2D& extent, std::span<const vk::ClearValue> clear_values);
	void end(vk::CommandBuffer command_buffer);
	void next(vk::CommandBuffer command_buffer);

	vk::RenderPass get() const;
	uint32_t       getFramebufferCount() const;
};

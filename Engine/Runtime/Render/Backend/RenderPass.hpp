#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"

struct RenderPassConfig {
	std::vector<vk::AttachmentDescription> attachments;
	std::vector<vk::SubpassDescription>    subpasses;
	std::vector<vk::SubpassDependency>     dependencies;
};

class RenderPass {
private:
	vk::RenderPass render_pass;

	std::vector<vk::Framebuffer> framebuffers;

	Context* context{};

	void create(const RenderPassConfig& config);

public:
	RenderPass(Context& context, const RenderPassConfig& config);
	~RenderPass();

	RenderPass(const RenderPass&) = delete;
	RenderPass& operator=(const RenderPass&) = delete;

	RenderPass(RenderPass&&) noexcept = default;
	RenderPass& operator=(RenderPass&&) noexcept = default;

	void createFramebuffers(std::span<const std::vector<vk::ImageView>> attachments_per_frame, vk::Extent2D extent);

	void begin(vk::CommandBuffer command_buffer, uint32_t framebuffer_index,
	    const vk::Extent2D& extent, std::span<const vk::ClearValue> clear_values);
	void end(vk::CommandBuffer command_buffer);
	void next(vk::CommandBuffer command_buffer);

	vk::RenderPass get() const;
	uint32_t       getFramebufferCount() const;
};

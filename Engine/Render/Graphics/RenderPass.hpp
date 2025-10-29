#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"
#include "SwapChain.hpp"

struct RenderPassConfig {
	std::vector<vk::AttachmentDescription> attachments = {
	    vk::AttachmentDescription()
	        .setFormat(vk::Format::eB8G8R8A8Srgb)
	        .setSamples(vk::SampleCountFlagBits::e1)
	        .setLoadOp(vk::AttachmentLoadOp::eClear)
	        .setStoreOp(vk::AttachmentStoreOp::eStore)
	        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	        .setInitialLayout(vk::ImageLayout::eUndefined)
	        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
	};

	std::vector<vk::SubpassDescription> subpasses = {
	    vk::SubpassDescription()
	        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
	        .setColorAttachments(
	            vk::AttachmentReference()
	                .setAttachment(0)
	                .setLayout(vk::ImageLayout::eColorAttachmentOptimal)),
	};

	std::vector<vk::SubpassDependency> dependencies = {
	    vk::SubpassDependency()
	        .setSrcSubpass(vk::SubpassExternal)
	        .setDstSubpass(0)
	        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
	        .setSrcAccessMask(vk::AccessFlagBits::eNone)
	        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
	        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite),

	};
};

class RenderPass {
private:
	vk::RenderPass render_pass;

	std::vector<vk::Framebuffer> framebuffers;

	RenderPassConfig config;

	Context*   context{};
	SwapChain* swap_chain{};

	void create(const RenderPassConfig& config);

	void createFrameBuffers(std::span<const vk::ImageView> attachments, vk::Extent2D extent, uint32_t count);

public:
	RenderPass(Context& context, SwapChain& swap_chain, const RenderPassConfig& config = {});

	RenderPass(const RenderPass&) = delete;
	RenderPass& operator=(const RenderPass&) = delete;

	RenderPass(RenderPass&&) noexcept = default;
	RenderPass& operator=(RenderPass&&) noexcept = default;

	~RenderPass();

	void begin(vk::CommandBuffer command_buffer, uint32_t framebuffer_index, const vk::Extent2D& extent, const vk::ClearValue& color);
	void end(vk::CommandBuffer command_buffer);
	void next(vk::CommandBuffer command_buffer);

	vk::RenderPass get() const;

	const RenderPassConfig& getConfig() const;
};

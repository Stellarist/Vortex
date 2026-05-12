#pragma once

#include "Runtime/Render/Backend/VulkanRenderPass.hpp"

enum class PassType : uint32_t {
	Base = 0,
	Forward,
	Geometry,
	Lighting,
	Shadow,
	Count,
};

class BasePass {
protected:
	PassType type{};

	vk::Extent2D extent{};

	std::unique_ptr<VulkanRenderPass> pass;

	VulkanContext* context{};

public:
	BasePass();
	virtual ~BasePass() = default;

	virtual void initialize(VulkanContext& context, vk::Extent2D extent) = 0;
	virtual void cleanup() = 0;
	virtual void resize(vk::Extent2D new_extent) = 0;

	virtual void begin(vk::CommandBuffer command_buffer, uint32_t frame_index,
	    std::span<const vk::ClearValue> clear_values);
	virtual void end(vk::CommandBuffer command_buffer);

	PassType getType() const;

	vk::Extent2D getExtent() const;
	VulkanRenderPass&  getPass();
};

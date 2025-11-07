#pragma once

#include "Render/Graphics/RenderPass.hpp"

enum class PassType : uint32_t {
	Base = 0,
	Forward,
	Geometry,
	Lighting,
	Count
};

class BasePass {
protected:
	PassType type{};

	vk::Extent2D extent{};

	std::unique_ptr<RenderPass> pass;

	Context* context{};

public:
	BasePass();
	virtual ~BasePass() = default;

	virtual void initialize(Context& context, vk::Extent2D extent) = 0;
	virtual void cleanup() = 0;
	virtual void resize(vk::Extent2D new_extent) = 0;

	virtual void begin(vk::CommandBuffer command_buffer, uint32_t frame_index,
	    std::span<const vk::ClearValue> clear_values);
	virtual void end(vk::CommandBuffer command_buffer);

	PassType getType() const;

	vk::Extent2D getExtent() const;
	RenderPass&  getPass();
};

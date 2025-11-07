#include "BasePass.hpp"

BasePass::BasePass() :
    type(PassType::Base)
{}

void BasePass::begin(vk::CommandBuffer command_buffer,
    uint32_t                           frame_index,
    std::span<const vk::ClearValue>    clear_values)
{
	pass->begin(command_buffer, frame_index, extent, clear_values);
}

void BasePass::end(vk::CommandBuffer command_buffer)
{
	pass->end(command_buffer);
}

PassType BasePass::getType() const
{
	return type;
}

vk::Extent2D BasePass::getExtent() const
{
	return extent;
}

RenderPass& BasePass::getPass()
{
	return *pass;
}

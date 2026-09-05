module;

#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>

module Runtime.Vulkan;

import vulkan;

namespace Vortex {

VulkanDevice::VulkanDevice(VulkanContext& context, vk::Device device) :
    device(device), context(&context)
{
	set_debug_name = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
	    vkGetInstanceProcAddr(static_cast<VkInstance>(context.getInstance()), "vkSetDebugUtilsObjectNameEXT"));
	begin_debug_label = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
	    vkGetInstanceProcAddr(static_cast<VkInstance>(context.getInstance()), "vkCmdBeginDebugUtilsLabelEXT"));
	end_debug_label = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
	    vkGetInstanceProcAddr(static_cast<VkInstance>(context.getInstance()), "vkCmdEndDebugUtilsLabelEXT"));

	VmaAllocatorCreateInfo create_info{
	    .physicalDevice = context.getPhysicalDevice(),
	    .device = device,
	    .instance = context.getInstance(),
	    .vulkanApiVersion = VK_API_VERSION_1_4,
	};

	const auto result = vmaCreateAllocator(&create_info, &allocator);
	CHECK(result == VK_SUCCESS, "Failed to create Vulkan memory allocator (result {})",
	    static_cast<int32>(result));

	queue = std::make_unique<VulkanQueue>(*this, RHICommandQueue::Graphics, context.getQueueIndices().graphics_family.value());
	LOG(Debug, "Vulkan device initialized");
}

VulkanDevice::~VulkanDevice() noexcept
{
	device.waitIdle();
	queue.reset();
	vmaDestroyAllocator(allocator);
	device.destroy();
	device = vk::Device{};
	LOG(Debug, "Vulkan device shut down");
}

void VulkanDevice::beginDebugLabel(vk::CommandBuffer command, std::string_view name) const noexcept
{
	if (!begin_debug_label || !command || name.empty())
		return;

	const std::string label_name(name);
	const VkDebugUtilsLabelEXT label{
	    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
	    .pLabelName = label_name.c_str(),
	    .color = {0.18f, 0.45f, 0.72f, 1.0f},
	};

	begin_debug_label(static_cast<VkCommandBuffer>(command), &label);
}

void VulkanDevice::endDebugLabel(vk::CommandBuffer command) const noexcept
{
	if (end_debug_label && command)
		end_debug_label(static_cast<VkCommandBuffer>(command));
}

}        // namespace Vortex

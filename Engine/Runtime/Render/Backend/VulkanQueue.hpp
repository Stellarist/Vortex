#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanCommand.hpp"
#include "Runtime/Render/RHI/RHITypes.hpp"

struct VulkanSubmission {
	uint64_t value{};

	std::vector<std::shared_ptr<VulkanCommandBuffer>> commands{};
};

class VulkanQueue {
private:
	RHICommandQueue type{};

	vk::Queue     queue{};
	vk::Semaphore timeline_semaphore{};

	std::unique_ptr<VulkanCommandPool> command_pool{};

	std::deque<VulkanSubmission> pending_submissions{};

	uint32_t family_index{};
	uint64_t next_timeline_value{1};
	uint64_t completed_timeline_value{0};

	VulkanDevice& device;

	void updateCompletedTimeline();
	void retireCompletedCommands();

public:
	VulkanQueue(VulkanDevice& device, RHICommandQueue type, uint32_t family_index);
	~VulkanQueue();

	std::shared_ptr<VulkanCommandBuffer> acquireCommand();

	void submit(VulkanCommandList*              command_list,
	    std::span<const vk::Semaphore>          wait_semaphores = {},
	    std::span<const vk::PipelineStageFlags> wait_stages = {},
	    std::span<const vk::Semaphore>          signal_semaphores = {},
	    vk::Fence                               fence = {});

	RHICommandQueue getType() const { return type; }

	uint32_t getFamilyIndex() const { return family_index; }

	vk::Queue     getHandle() const { return queue; }
	vk::Semaphore getSemaphore() const { return timeline_semaphore; }
};

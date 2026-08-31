export module Runtime.Graphics:Vulkan.Queue;

import vulkan;
import Core;
import :Vulkan.Command;
import :RHI.Types;

export namespace Vortex {

struct VulkanSubmission {
	uint64 value{};

	std::vector<std::shared_ptr<VulkanCommandBuffer>> commands{};
};

class VulkanQueue {
private:
	RHICommandQueue type{};

	vk::Queue     queue{};
	vk::Semaphore timeline_semaphore{};

	std::unique_ptr<VulkanCommandPool> command_pool{};

	std::deque<VulkanSubmission> pending_submissions{};

	uint32 family_index{};
	uint64 next_timeline_value{1};
	uint64 completed_timeline_value{0};

	VulkanDevice& device;

	void updateCompletedTimeline();
	void retireCompletedCommands();

public:
	VulkanQueue(VulkanDevice& device, RHICommandQueue type, uint32 family_index);
	~VulkanQueue();

	std::shared_ptr<VulkanCommandBuffer> acquireCommand();

	void submit(VulkanCommandList*              command_list,
	    std::span<const vk::Semaphore>          wait_semaphores = {},
	    std::span<const vk::PipelineStageFlags> wait_stages = {},
	    std::span<const vk::Semaphore>          signal_semaphores = {},
	    vk::Fence                               fence = {});

	RHICommandQueue getType() const noexcept { return type; }

	uint32 getFamilyIndex() const noexcept { return family_index; }

	vk::Queue getHandle() const noexcept { return queue; }
	vk::Semaphore getSemaphore() const noexcept { return timeline_semaphore; }
};

}        // namespace Vortex

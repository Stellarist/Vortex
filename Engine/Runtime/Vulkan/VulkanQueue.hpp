export module Runtime.Vulkan:Queue;

import vulkan;
import Core;
import :Command;
import Runtime.RHI;

export namespace Vortex {

struct VulkanSubmission {
	uint64 value{};

	std::vector<std::shared_ptr<VulkanCommandBuffer>> commands{};
};

class VulkanQueue {
private:
	RHICommandQueue type{};

	vk::Queue queue{};
	vk::Semaphore timeline_semaphore{};

	std::unique_ptr<VulkanCommandPool> command_pool{};
	std::deque<VulkanSubmission> pending_submissions{};

	uint32 family_index{};
	uint64 next_timeline_value{1};
	uint64 completed_timeline_value{0};

	VulkanDevice& device;

public:
	VulkanQueue(VulkanDevice& device, RHICommandQueue type, uint32 family_index);
	~VulkanQueue() noexcept;

	std::shared_ptr<VulkanCommandBuffer> acquire();

	void collect();
	void submit(VulkanCommandList* command_list,
	    std::span<const vk::SemaphoreSubmitInfo> waits = {},
	    std::span<const vk::SemaphoreSubmitInfo> signals = {},
	    vk::Fence fence = {});

	RHICommandQueue getType() const noexcept { return type; }

	uint32 getFamilyIndex() const noexcept { return family_index; }

	vk::Queue getHandle() const noexcept { return queue; }
	vk::Semaphore getSemaphore() const noexcept { return timeline_semaphore; }
};

}        // namespace Vortex

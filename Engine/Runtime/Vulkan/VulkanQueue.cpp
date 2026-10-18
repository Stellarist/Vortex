module Runtime.Vulkan;

import vulkan;

namespace Vortex {

VulkanQueue::VulkanQueue(VulkanDevice& device, RHICommandQueue type, uint32 family_index) :
    type(type), family_index(family_index), device(device)
{
	vk::SemaphoreTypeCreateInfo type_info{};
	type_info.setSemaphoreType(vk::SemaphoreType::eTimeline)
	    .setInitialValue(0);

	vk::SemaphoreCreateInfo create_info{};
	create_info.setPNext(&type_info);

	timeline_semaphore = device.getHandle().createSemaphore(create_info);
	queue = device.getHandle().getQueue(family_index, 0);
	command_pool = std::make_unique<VulkanCommandPool>(device, *this);
	LOG(Debug, "Vulkan queue ready (family {})", family_index);
}

VulkanQueue::~VulkanQueue() noexcept
{
	if (queue)
		queue.waitIdle();

	pending_submissions.clear();
	command_pool.reset();

	if (timeline_semaphore)
		device.getHandle().destroySemaphore(timeline_semaphore);
}

std::shared_ptr<VulkanCommandBuffer> VulkanQueue::acquire()
{
	collect();

	auto* command = command_pool->createCommandBuffer();
	command->closed = command->submitted = false;
	return {command, [this](VulkanCommandBuffer* command) {
		        command_pool->releaseCommandBuffer(command);
	        }};
}

void VulkanQueue::submit(VulkanCommandList* command_list,
    std::span<const vk::SemaphoreSubmitInfo> waits,
    std::span<const vk::SemaphoreSubmitInfo> signals,
    vk::Fence fence)
{
	CHECK(command_list && &command_list->device == &device, "Vulkan command list belongs to a different device");

	auto command = command_list->getCurrentCommand();
	CHECK(command && command->closed && !command->submitted, "Vulkan submission requires a closed, unsubmitted command list");
	collect();

	const uint64 signal_value = next_timeline_value;
	vk::CommandBufferSubmitInfo command_info{};
	command_info.setCommandBuffer(command->getHandle());

	std::vector<vk::SemaphoreSubmitInfo> submit_signals(signals.begin(), signals.end());
	submit_signals.emplace_back()
	    .setSemaphore(timeline_semaphore)
	    .setValue(signal_value)
	    .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

	vk::SubmitInfo2 submit_info{};
	submit_info.setWaitSemaphoreInfos(waits)
	    .setCommandBufferInfos(command_info)
	    .setSignalSemaphoreInfos(submit_signals);

	pending_submissions.push_back({signal_value, {command}});
	const auto result = queue.submit2(1, &submit_info, fence);
	if (result != vk::Result::eSuccess)
		pending_submissions.pop_back();

	CHECK(result == vk::Result::eSuccess, "Failed to submit Vulkan command list (result {})", static_cast<int32>(result));
	command->submitted = true;
	++next_timeline_value;
}

void VulkanQueue::collect()
{
	completed_timeline_value = std::max(completed_timeline_value,
	    device.getHandle().getSemaphoreCounterValue(timeline_semaphore));

	while (!pending_submissions.empty() && pending_submissions.front().value <= completed_timeline_value) {
		for (const auto& command : pending_submissions.front().commands)
			command->resetResources();
		pending_submissions.pop_front();
	}
}

}        // namespace Vortex

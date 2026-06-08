#include "VulkanQueue.hpp"

VulkanQueue::VulkanQueue(VulkanDevice& device, RHICommandQueue type, uint32_t family_index) :
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
}

VulkanQueue::~VulkanQueue()
{
	if (queue)
		queue.waitIdle();

	pending_submissions.clear();
	command_pool.reset();

	if (timeline_semaphore)
		device.getHandle().destroySemaphore(timeline_semaphore);
}

std::shared_ptr<VulkanCommandBuffer> VulkanQueue::acquireCommand()
{
	retireCompletedCommands();

	auto* command = command_pool->createCommandBuffer();
	return {command, [this](VulkanCommandBuffer* command) {
		        command_pool->releaseCommandBuffer(command);
	        }};
}

void VulkanQueue::submit(VulkanCommandList* command_list,
    std::span<const vk::Semaphore>          wait_semaphores,
    std::span<const vk::PipelineStageFlags> wait_stages,
    std::span<const vk::Semaphore>          signal_semaphores,
    vk::Fence                               fence)
{
	retireCompletedCommands();

	assert(!command_list || command_list->getDesc().queue_type == type && "Cannot submit a command list that belongs to a different queue type.");

	auto command = command_list->getCurrentCommand();
	if (!command)
		throw std::runtime_error("Cannot submit a command list without a recorded command buffer.");

	std::array command_buffers{command->getHandle()};
	uint64_t   signal_value = ++next_timeline_value;

	std::vector<vk::Semaphore> submit_signal_semaphores(signal_semaphores.begin(), signal_semaphores.end());
	submit_signal_semaphores.push_back(timeline_semaphore);

	std::vector<uint64_t> signal_values(submit_signal_semaphores.size(), 0);
	signal_values.back() = signal_value;

	std::vector<uint64_t>               wait_values(wait_semaphores.size(), 0);
	std::vector<vk::PipelineStageFlags> submit_wait_stages(wait_stages.begin(), wait_stages.end());
	if (!wait_semaphores.empty() && submit_wait_stages.empty())
		submit_wait_stages.assign(wait_semaphores.size(), vk::PipelineStageFlagBits::eAllCommands);

	assert(wait_semaphores.size() == submit_wait_stages.size() && "Queue submit wait stage count must match wait semaphore count.");

	vk::TimelineSemaphoreSubmitInfo timeline_info{};
	timeline_info.setWaitSemaphoreValues(wait_values)
	    .setSignalSemaphoreValues(signal_values);

	vk::SubmitInfo submit_info{};
	submit_info.setPNext(&timeline_info)
	    .setWaitSemaphores(wait_semaphores)
	    .setWaitDstStageMask(submit_wait_stages)
	    .setCommandBuffers(command_buffers)
	    .setSignalSemaphores(submit_signal_semaphores);

	queue.submit(submit_info, fence);

	pending_submissions.push_back({signal_value, {std::move(command)}});
}

void VulkanQueue::updateCompletedTimeline()
{
	completed_timeline_value = std::max(completed_timeline_value,
	    device.getHandle().getSemaphoreCounterValue(timeline_semaphore));
}

void VulkanQueue::retireCompletedCommands()
{
	updateCompletedTimeline();

	while (!pending_submissions.empty() && pending_submissions.front().value <= completed_timeline_value)
		pending_submissions.pop_front();
}

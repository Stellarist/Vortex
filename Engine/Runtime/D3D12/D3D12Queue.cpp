module Runtime.D3D12;

import d3d12;

namespace Vortex {

D3D12Queue::D3D12Queue(D3D12Device& device) : device(device)
{
	const D3D12_COMMAND_QUEUE_DESC desc{.Type = D3D12_COMMAND_LIST_TYPE_DIRECT};

	const HRESULT queue_result = device.getHandle()->CreateCommandQueue(&desc, IID_ID3D12CommandQueue, &queue);
	CHECK(SUCCEEDED(queue_result), "CreateCommandQueue failed (HRESULT 0x{:08X})", static_cast<uint32>(queue_result));

	const HRESULT fence_result = device.getHandle()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_ID3D12Fence, &fence);
	CHECK(SUCCEEDED(fence_result), "CreateFence failed (HRESULT 0x{:08X})", static_cast<uint32>(fence_result));

	event = CreateEventW(nullptr, false, false, nullptr);
	CHECK(event, "Cannot create D3D12 fence event");
}

D3D12Queue::~D3D12Queue() noexcept
{
	if (event)
		CloseHandle(event);
}

void D3D12Queue::collect()
{
	const auto completed = fence->GetCompletedValue();
	CHECK(completed != ~uint64(0), "D3D12 device removed (HRESULT 0x{:08X})", static_cast<uint32>(device.getHandle()->GetDeviceRemovedReason()));

	while (!in_flight.empty() && in_flight.front()->submission <= completed) {
		auto command = std::move(in_flight.front());
		in_flight.pop_front();
		command->resetResources();
		available.push_back(std::move(command));
	}
}

std::shared_ptr<D3D12CommandBuffer> D3D12Queue::acquire()
{
	collect();

	std::shared_ptr<D3D12CommandBuffer> result;
	for (auto it = available.begin(); it != available.end(); ++it)
		if (it->use_count() == 1) {
			result = std::move(*it);
			available.erase(it);
			break;
		}

	if (result)
		result->reset();
	else
		result = std::make_shared<D3D12CommandBuffer>(device.getHandle());

	return result;
}

uint64 D3D12Queue::submit(D3D12CommandList* command_list)
{
	CHECK(command_list && &command_list->device == &device, "D3D12 command list belongs to a different device");
	auto command = command_list->getCurrentCommand();

	CHECK(command && command->closed && !command->submission, "D3D12 submission requires a closed, unsubmitted command list");
	collect();

	ID3D12CommandList* lists[] = {command->command.Get()};
	in_flight.push_back(command);
	command->submission = submitted + 1;
	queue->ExecuteCommandLists(1, lists);
	signal();

	return command->submission;
}

uint64 D3D12Queue::signal()
{
	const HRESULT result = queue->Signal(fence.Get(), ++submitted);
	CHECK(SUCCEEDED(result), "Signal queue fence failed (HRESULT 0x{:08X})", static_cast<uint32>(result));

	return submitted;
}

void D3D12Queue::wait(uint64 value)
{
	if (fence->GetCompletedValue() < value) {
		const HRESULT result = fence->SetEventOnCompletion(value, event);
		CHECK(SUCCEEDED(result), "SetEventOnCompletion failed (HRESULT 0x{:08X})", static_cast<uint32>(result));
		CHECK(WaitForSingleObject(event, INFINITE) == WAIT_OBJECT_0, "D3D12 fence wait failed");
	}

	collect();
}

HRESULT D3D12Queue::waitIdle() noexcept
{
	const uint64 value = ++submitted;
	const HRESULT result = queue->Signal(fence.Get(), value);
	if (!SUCCEEDED(result))
		return result;

	const HRESULT wait_result = fence->SetEventOnCompletion(value, nullptr);
	return SUCCEEDED(wait_result) ? device.getHandle()->GetDeviceRemovedReason() : wait_result;
}

}        // namespace Vortex

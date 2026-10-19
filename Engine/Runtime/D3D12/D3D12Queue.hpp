export module Runtime.D3D12:Queue;

import d3d12;
import Core;
import Runtime.RHI;
import :Types;
import :Device;
import :Command;

export namespace Vortex {

class D3D12Queue {
private:
	D3D12Ptr<ID3D12CommandQueue> queue{};
	D3D12Ptr<ID3D12Fence> fence{};

	HANDLE event{};
	uint64 submitted{};

	std::deque<std::shared_ptr<D3D12CommandBuffer>> in_flight{};
	std::vector<std::shared_ptr<D3D12CommandBuffer>> available{};

	void collect();

	D3D12Device& device;

public:
	D3D12Queue(D3D12Device& device);
	~D3D12Queue() noexcept;

	std::shared_ptr<D3D12CommandBuffer> acquire();
	uint64 submit(D3D12CommandList* command_list);

	uint64 signal();
	void wait(uint64 value);
	HRESULT waitIdle() noexcept;

	ID3D12CommandQueue* getHandle() const noexcept { return queue.Get(); }
};

}        // namespace Vortex

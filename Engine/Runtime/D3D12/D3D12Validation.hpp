export module Runtime.D3D12:Validation;

import d3d12;
import Core;
import :Types;

export namespace Vortex {

class D3D12ValidationLayer {
private:
	D3D12Ptr<ID3D12InfoQueue1> info_queue{};
	std::optional<DWORD> callback_cookie{};

	std::atomic_uint32_t error_count{};
	bool available{};

public:
	D3D12ValidationLayer();
	~D3D12ValidationLayer() noexcept;

	void attach(ID3D12Device& device);

	bool isAvailable() const noexcept { return available; }
	bool isEnabled() const noexcept { return callback_cookie.has_value(); }
	uint32 getErrorCount() const noexcept { return error_count.load(std::memory_order_relaxed); }
};

}        // namespace Vortex

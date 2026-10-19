module Runtime.D3D12;

import d3d12;

namespace Vortex {

#if VDEBUG
static void __stdcall vortexValidationCallback(
    D3D12_MESSAGE_CATEGORY message_category,
    D3D12_MESSAGE_SEVERITY message_severity,
    D3D12_MESSAGE_ID message_id,
    const char* description,
    void* user_data) noexcept
{
	const bool is_error = message_severity == D3D12_MESSAGE_SEVERITY_CORRUPTION ||
	    message_severity == D3D12_MESSAGE_SEVERITY_ERROR;
	if (is_error && user_data)
		static_cast<std::atomic_uint32_t*>(user_data)->fetch_add(1, std::memory_order_relaxed);

	static_cast<void>(message_category);
	static_cast<void>(message_id);
	const std::string_view message = description && *description ?
	    description :
	    "D3D12 validation produced an empty message";
	try {
		if (is_error)
			LOG(Error, "D3D12 validation: {}", message);
		else if (message_severity == D3D12_MESSAGE_SEVERITY_WARNING)
			LOG(Warn, "D3D12 validation: {}", message);
		else
			LOG(Debug, "D3D12 validation: {}", message);
	} catch (...) {
	}
}
#endif

D3D12ValidationLayer::D3D12ValidationLayer()
{
#if VDEBUG
	D3D12Ptr<ID3D12Debug> debug{};
	const HRESULT result = D3D12GetDebugInterface(IID_ID3D12Debug, &debug);
	if (SUCCEEDED(result)) {
		debug->EnableDebugLayer();
		available = true;
	} else {
		LOG(Warn, "D3D12 debug layer unavailable; install Windows Graphics Tools to enable validation");
	}
#endif
}

D3D12ValidationLayer::~D3D12ValidationLayer() noexcept
{
#if VDEBUG
	if (callback_cookie)
		info_queue->UnregisterMessageCallback(*callback_cookie);
#endif
}

void D3D12ValidationLayer::attach(ID3D12Device& device)
{
#if VDEBUG
	if (!available)
		return;

	const HRESULT query_result = device.QueryInterface(IID_ID3D12InfoQueue1, &info_queue);
	CHECK(SUCCEEDED(query_result), "Query ID3D12InfoQueue1 failed (HRESULT 0x{:08X})", static_cast<uint32>(query_result));
	DWORD cookie{};
	const HRESULT result = info_queue->RegisterMessageCallback(
	    vortexValidationCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, &error_count, &cookie);
	CHECK(SUCCEEDED(result), "Register D3D12 validation callback failed (HRESULT 0x{:08X})", static_cast<uint32>(result));
	callback_cookie = cookie;
#else
	static_cast<void>(device);
#endif
}

}        // namespace Vortex

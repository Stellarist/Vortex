module;

#include <SDL3/SDL.h>

module Runtime.D3D12;

import d3d12;

namespace Vortex {

D3D12Context::D3D12Context(Window& window) : window(&window)
{
	const auto hwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window.get()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
	CHECK(hwnd, "SDL window has no Win32 HWND");
	const uint32 factory_flags = validation.isAvailable() ? DXGI_CREATE_FACTORY_DEBUG : 0;

	const HRESULT factory_result = CreateDXGIFactory2(factory_flags, IID_IDXGIFactory6, &factory);
	CHECK(SUCCEEDED(factory_result), "CreateDXGIFactory2 failed (HRESULT 0x{:08X})", static_cast<uint32>(factory_result));

	D3D12Ptr<ID3D12Device> native_device;
	for (uint32 index = 0;; ++index) {
		D3D12Ptr<IDXGIAdapter1> candidate;
		const HRESULT result = factory->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_IDXGIAdapter1, &candidate);
		if (result == DXGI_ERROR_NOT_FOUND)
			break;
		CHECK(SUCCEEDED(result), "EnumAdapterByGpuPreference failed (HRESULT 0x{:08X})", static_cast<uint32>(result));

		DXGI_ADAPTER_DESC1 desc{};
		const HRESULT desc_result = candidate->GetDesc1(&desc);
		CHECK(SUCCEEDED(desc_result), "Get adapter description failed (HRESULT 0x{:08X})", static_cast<uint32>(desc_result));
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		const HRESULT device_result = D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_11_0, IID_ID3D12Device, &native_device);
		if (SUCCEEDED(device_result)) {
			adapter = candidate;
			break;
		}
	}

	CHECK(native_device, "No D3D12-capable hardware adapter found");
	validation.attach(*native_device.Get());

	device = std::make_unique<D3D12Device>(*this, std::move(native_device));
	uint32 width{}, height{};
	window.getPixelSize(width, height);
	presentation = std::make_unique<D3D12Presentation>(*device, *factory.Get(), hwnd, RHIExtent{width, height});

	LOG("D3D12 initialized (validation {})", validation.isEnabled() ? "enabled" : "disabled");
}

D3D12Context::~D3D12Context() noexcept
{
	if (device)
		device->waitIdle();

	presentation.reset();
	device.reset();
}

bool D3D12Context::beginFrame()
{
	uint32 width{}, height{};
	window->getPixelSize(width, height);
	if (window->isMinimized() || !width || !height)
		return false;
	return presentation->beginFrame({width, height});
}

void D3D12Context::endFrame()
{
	presentation->endFrame();
}

RHICommandList& D3D12Context::getCommand() noexcept
{
	return presentation->getCommand();
}

RHITexture& D3D12Context::getBackbuffer() noexcept
{
	return presentation->getBackbuffer();
}

RHITextureView& D3D12Context::getBackbufferView() noexcept
{
	return presentation->getBackbufferView();
}

RHIDevice& D3D12Context::getDevice() noexcept
{
	return *device;
}

RHIFormat D3D12Context::getFormat() const noexcept
{
	return presentation->getFormat();
}

RHIExtent D3D12Context::getExtent() const noexcept
{
	return presentation->getExtent();
}

}        // namespace Vortex

export module Runtime.D3D12:Context;

import d3d12;
import Core;
import Editor.Window;
import Runtime.RHI;
import :Types;
import :Validation;

export namespace Vortex {

class D3D12Device;
class D3D12Presentation;

class D3D12Context : public RHIContext {
private:
	D3D12ValidationLayer validation{};

	D3D12Ptr<IDXGIFactory6> factory{};
	D3D12Ptr<IDXGIAdapter1> adapter{};

	std::unique_ptr<D3D12Device> device{};
	std::unique_ptr<D3D12Presentation> presentation{};

	Window* window{};

public:
	D3D12Context(Window& window);
	~D3D12Context() noexcept override;

	bool beginFrame() override;
	void endFrame() override;

	RHICommandList& getCommand() noexcept override;
	RHITexture& getBackbuffer() noexcept override;
	RHITextureView& getBackbufferView() noexcept override;

	RHIDevice& getDevice() noexcept override;

	RHIFormat getFormat() const noexcept override;
	RHIExtent getExtent() const noexcept override;

	uint32 getValidationErrorCount() const noexcept { return validation.getErrorCount(); }

	IDXGIFactory6* getFactory() const noexcept { return factory.Get(); }
	IDXGIAdapter1* getAdapter() const noexcept { return adapter.Get(); }
};

}        // namespace Vortex

export module Runtime.D3D12:Presentation;

import d3d12;
import Core;
import Runtime.RHI;
import :Types;
import :Device;

namespace Vortex {

class D3D12Presentation {
private:
	struct SubmissionSlot {
		RHIRef<RHICommandList> command{};
		uint64 in_flight{};
	};

	struct PresentImage {
		D3D12Ptr<ID3D12Resource> image{};
		RHIRef<RHITexture> backbuffer{};
		RHIRef<RHITextureView> backbuffer_view{};
		uint64 in_flight{};
	};

	D3D12Ptr<IDXGISwapChain3> swapchain{};

	RHIContextDesc desc{.api = RHIAPI::D3D12};

	std::array<SubmissionSlot, RHIMaxFramesInFlight> submission_slots{};
	std::array<PresentImage, RHIPreferredSwapchainImageCount> present_images{};

	uint32 submission_slot_index{};
	uint32 acquired_image_index{};

	bool frame_active{};

	D3D12Device& device;

	void createSwapchain(IDXGIFactory6& factory, HWND window, const RHIExtent& extent);
	bool recreateSwapchain(const RHIExtent& extent);
	void createResources();
	void destroyResources() noexcept;

public:
	D3D12Presentation(D3D12Device& device,
	    IDXGIFactory6& factory,
	    HWND window,
	    const RHIExtent& extent);
	~D3D12Presentation() noexcept;

	bool beginFrame(const RHIExtent& extent);
	void endFrame();

	RHICommandList& getCommand() noexcept { return *submission_slots[submission_slot_index].command; }
	RHITexture& getBackbuffer() noexcept { return *present_images[acquired_image_index].backbuffer; }
	RHITextureView& getBackbufferView() noexcept { return *present_images[acquired_image_index].backbuffer_view; }

	RHIFormat getFormat() const noexcept { return desc.format; }
	RHIExtent getExtent() const noexcept { return desc.extent; }
};

}        // namespace Vortex

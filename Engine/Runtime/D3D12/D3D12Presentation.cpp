module Runtime.D3D12;

import d3d12;

namespace Vortex {

D3D12Presentation::D3D12Presentation(D3D12Device& device, IDXGIFactory6& factory, HWND hwnd, const RHIExtent& initial_extent) : device(device)
{
	createSwapchain(factory, hwnd, initial_extent);
}

D3D12Presentation::~D3D12Presentation() noexcept
{
	destroyResources();
}

void D3D12Presentation::createSwapchain(IDXGIFactory6& factory, HWND window, const RHIExtent& extent)
{
	const auto [width, height] = extent;

	const DXGI_SWAP_CHAIN_DESC1 swapchain_desc{
	    .Width = std::max(1u, width),
	    .Height = std::max(1u, height),
	    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	    .SampleDesc = {.Count = 1},
	    .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
	    .BufferCount = static_cast<UINT>(present_images.size()),
	    .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
	};

	D3D12Ptr<IDXGISwapChain1> created;
	const HRESULT swapchain_result = factory.CreateSwapChainForHwnd(device.getQueue().getHandle(), window, &swapchain_desc, nullptr, nullptr, &created);
	CHECK(SUCCEEDED(swapchain_result), "CreateSwapChainForHwnd failed (HRESULT 0x{:08X})", static_cast<uint32>(swapchain_result));

	const HRESULT query_result = created.As(&swapchain);
	CHECK(SUCCEEDED(query_result), "Query IDXGISwapChain3 failed (HRESULT 0x{:08X})", static_cast<uint32>(query_result));

	const HRESULT association_result = factory.MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
	CHECK(SUCCEEDED(association_result), "MakeWindowAssociation failed (HRESULT 0x{:08X})", static_cast<uint32>(association_result));

	desc.extent = {swapchain_desc.Width, swapchain_desc.Height};
	createResources();

	LOG("D3D12 swapchain ready ({}x{}, {} images, {} submission slots)",
	    desc.extent.width, desc.extent.height, present_images.size(), submission_slots.size());
}

bool D3D12Presentation::recreateSwapchain(const RHIExtent& extent)
{
	if (!extent.width || !extent.height)
		return false;

	device.waitIdle();
	destroyResources();

	const HRESULT result = swapchain->ResizeBuffers(static_cast<UINT>(present_images.size()), extent.width, extent.height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	CHECK(SUCCEEDED(result), "ResizeBuffers failed (HRESULT 0x{:08X})", static_cast<uint32>(result));
	desc.extent = extent;
	createResources();

	return true;
}

void D3D12Presentation::createResources()
{
	for (auto& submission : submission_slots)
		submission.command = device.createCommandList(RHICommandListDesc{});

	RHITextureDesc backbuffer_desc{};
	backbuffer_desc.setWidth(desc.extent.width)
	    .setHeight(desc.extent.height)
	    .setFormat(desc.format)
	    .setUsage(RHITextureUsage::RenderTarget | RHITextureUsage::TransferSource | RHITextureUsage::TransferDest);

	for (uint32 index = 0; index < present_images.size(); ++index) {
		auto& image = present_images[index];
		const HRESULT result = swapchain->GetBuffer(index, IID_ID3D12Resource, &image.image);
		CHECK(SUCCEEDED(result), "Get swapchain buffer failed (HRESULT 0x{:08X})", static_cast<uint32>(result));

		image.backbuffer = device.createTexture(backbuffer_desc);
		image.backbuffer->setName(std::format("D3D12.FrameOutput.{}", index));
		image.backbuffer_view = device.createTextureView(RHITextureViewDesc{}
		        .setTexture(image.backbuffer.get())
		        .setType(RHITextureViewType::RenderTarget));
	}
}

void D3D12Presentation::destroyResources() noexcept
{
	for (auto& submission : submission_slots)
		submission = {};
	for (auto& image : present_images)
		image = {};
	submission_slot_index = 0;
	acquired_image_index = 0;
	frame_active = false;
}

bool D3D12Presentation::beginFrame(const RHIExtent& surface_extent)
{
	CHECK(!frame_active, "D3D12 frame is already active");

	const auto [width, height] = surface_extent;
	if (!width || !height)
		return false;

	if (desc.extent != surface_extent && !recreateSwapchain(surface_extent))
		return false;

	acquired_image_index = swapchain->GetCurrentBackBufferIndex();
	auto& submission = submission_slots[submission_slot_index];
	const auto& image = present_images[acquired_image_index];

	device.getQueue().wait(std::max(submission.in_flight, image.in_flight));
	submission.command->open();
	frame_active = true;

	return true;
}

void D3D12Presentation::endFrame()
{
	CHECK(frame_active, "No active D3D12 frame to present");

	auto& submission = submission_slots[submission_slot_index];
	auto& image = present_images[acquired_image_index];
	auto& command = *submission.command;
	command.endRendering();
	command.transitionTexture(image.backbuffer.get(), image.backbuffer->getState(), TransferSource);

	auto* native = static_cast<D3D12CommandList*>(&command)->getCurrentCommand()->getHandle();
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition = {image.image.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST};
	native->ResourceBarrier(1, &barrier);
	native->CopyResource(image.image.Get(), static_cast<D3D12Texture*>(image.backbuffer.get())->getHandle());
	std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
	native->ResourceBarrier(1, &barrier);

	command.close();
	device.executeCommandList(&command);

	const HRESULT result = swapchain->Present(0, 0);
	CHECK(SUCCEEDED(result), "Present failed (HRESULT 0x{:08X})", static_cast<uint32>(result));
	submission.in_flight = device.getQueue().signal();
	image.in_flight = submission.in_flight;
	submission_slot_index = (submission_slot_index + 1) % RHIMaxFramesInFlight;
	frame_active = false;
}

}        // namespace Vortex

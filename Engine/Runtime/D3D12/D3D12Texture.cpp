module Runtime.D3D12;

import d3d12;

namespace Vortex {

static D3D12_TEXTURE_COPY_LOCATION textureLocation(D3D12Texture* texture, const RHITextureSlice& slice)
{
	D3D12_TEXTURE_COPY_LOCATION location{};
	location.pResource = texture->getHandle();
	location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	location.SubresourceIndex = slice.mip_level + slice.array_layer * texture->getDesc().mip_levels;

	return location;
}

static D3D12_BOX textureBox(const RHITextureSlice& s)
{
	return {static_cast<UINT>(s.x), static_cast<UINT>(s.y), static_cast<UINT>(s.z),
	    static_cast<UINT>(s.x + s.width), static_cast<UINT>(s.y + s.height), static_cast<UINT>(s.z + s.depth)};
}

RHIRef<RHITexture> D3D12Device::createTexture(const RHITextureDesc& desc)
{
	validateRHITextureDesc(desc);
	CHECK(!isRHISRGBFormat(desc.format) || (desc.usage & RHITextureUsage::Storage) == RHITextureUsage::None,
	    "D3D12 storage textures require a non-sRGB format");

	auto texture = makeRHIRef<D3D12Texture>(*this, desc);

	const auto native = toD3D12TextureDesc(desc);
	const D3D12MA::ALLOCATION_DESC allocation_desc{.HeapType = D3D12_HEAP_TYPE_DEFAULT};

	const auto clear = toD3D12ClearValue(desc);
	const bool attachment = (native.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0;
	const HRESULT result = allocator->CreateResource(&allocation_desc,
	    &native, D3D12_RESOURCE_STATE_COMMON, attachment ? &clear : nullptr,
	    &texture->allocation, IID_ID3D12Resource, &texture->texture);
	CHECK(SUCCEEDED(result), "Create texture failed (HRESULT 0x{:08X})", static_cast<uint32>(result));

	return texture;
}

RHIRef<RHITextureView> D3D12Device::createTextureView(const RHITextureViewDesc& desc)
{
	auto view = makeRHIRef<D3D12TextureView>(*this, normalizeRHITextureViewDesc(desc));
	const auto& normalized = view->getDesc();
	CHECK(normalized.subresource.base_array_layer == 0 ||
	        (normalized.dimension != RHITextureViewDimension::Texture1D &&
	            normalized.dimension != RHITextureViewDimension::Texture2D &&
	            normalized.dimension != RHITextureViewDimension::TextureCube),
	    "D3D12 non-array views cannot select a nonzero array layer; use an array view");

	if (desc.type == RHITextureViewType::RenderTarget || desc.type == RHITextureViewType::DepthStencil) {
		auto& descriptors = desc.type == RHITextureViewType::RenderTarget ? *render_target_descriptors : *depth_stencil_descriptors;
		view->descriptor = descriptors.allocateDescriptor();
		auto* texture = static_cast<D3D12Texture*>(normalized.texture.get());
		if (normalized.type == RHITextureViewType::RenderTarget) {
			const auto native = toD3D12RenderTargetViewDesc(normalized, texture->getDesc().sample_count);
			device->CreateRenderTargetView(texture->getHandle(), &native, view->getHandle());
		} else {
			const auto native = toD3D12DepthStencilViewDesc(normalized, texture->getDesc().sample_count);
			device->CreateDepthStencilView(texture->getHandle(), &native, view->getHandle());
		}
	}

	return view;
}

void D3D12Device::destroyTextureView(D3D12TextureView* view) noexcept
{
	if (view->descriptor == D3D12InvalidDescriptorIndex)
		return;
	auto& descriptors = view->desc.type == RHITextureViewType::RenderTarget ? *render_target_descriptors : *depth_stencil_descriptors;
	descriptors.releaseDescriptor(view->descriptor);
	view->descriptor = D3D12InvalidDescriptorIndex;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12TextureView::getHandle() const noexcept
{
	if (descriptor == D3D12InvalidDescriptorIndex)
		return {};
	const auto& descriptors = desc.type == RHITextureViewType::RenderTarget ? device.getRenderTargetDescriptors() : device.getDepthStencilDescriptors();
	return descriptors.getCpuHandle(descriptor);
}

RHIRef<RHIStagingTexture> D3D12Device::createStagingTexture(const RHITextureDesc& desc, RHIAccessMode access)
{
	validateRHITextureDesc(desc);
	CHECK(desc.sample_count == 1 && !hasRHIStencil(desc.format), "D3D12 staging requires a single-sample, single-plane format");
	CHECK(access == RHIAccessMode::Read || access == RHIAccessMode::Write, "Staging requires CPU read or write access");

	auto staging = makeRHIRef<D3D12StagingTexture>(*this, desc, access);
	const uint32 count = desc.mip_levels * desc.array_layers;
	staging->footprints.resize(count);

	auto native = toD3D12TextureDesc(desc);
	device->GetCopyableFootprints(&native, 0, count, 0, staging->footprints.data(), nullptr, nullptr, &staging->size);

	const D3D12MA::ALLOCATION_DESC allocation_desc{.HeapType = toD3D12HeapType(access)};
	const auto buffer_desc = toD3D12BufferDesc(staging->size);
	const auto state = getD3D12InitialBufferState(access);
	const HRESULT result = allocator->CreateResource(&allocation_desc, &buffer_desc, state, nullptr,
	    &staging->allocation, IID_ID3D12Resource, &staging->buffer);
	CHECK(SUCCEEDED(result), "Create staging buffer failed (HRESULT 0x{:08X})", static_cast<uint32>(result));

	return staging;
}

RHIRef<RHISampler> D3D12Device::createSampler(const RHISamplerDesc& desc)
{
	validateRHISamplerDesc(desc);

	return makeRHIRef<D3D12Sampler>(desc);
}

void* D3D12Device::mapStagingTexture(RHIStagingTexture* resource, RHIAccessMode mode,
    uint64& row_pitch, uint64& depth_pitch, uint32 mip, uint32 layer) const
{
	auto* staging = dynamic_cast<D3D12StagingTexture*>(resource);
	CHECK(staging && !staging->mapped && mode == staging->access && mip < staging->desc.mip_levels && layer < staging->desc.array_layers,
	    "Invalid D3D12 staging map access or subresource");

	void* data{};
	D3D12_RANGE range{0, mode == RHIAccessMode::Read ? static_cast<SIZE_T>(staging->size) : 0};
	const HRESULT result = staging->buffer->Map(0, &range, &data);
	CHECK(SUCCEEDED(result), "Map staging buffer failed (HRESULT 0x{:08X})", static_cast<uint32>(result));

	staging->mapped = true;
	const auto& layout = staging->footprints[mip + layer * staging->desc.mip_levels];
	row_pitch = layout.Footprint.RowPitch;
	depth_pitch = row_pitch * layout.Footprint.Height;

	return static_cast<std::byte*>(data) + layout.Offset;
}

void D3D12Device::unmapStagingTexture(RHIStagingTexture* resource) const noexcept
{
	auto* staging = dynamic_cast<D3D12StagingTexture*>(resource);
	if (!staging || !staging->mapped)
		return;
	D3D12_RANGE range{0, staging->access == RHIAccessMode::Write ? static_cast<SIZE_T>(staging->size) : 0};
	staging->buffer->Unmap(0, &range);
	staging->mapped = false;
}

void D3D12Device::destroyTexture(D3D12Texture* texture) noexcept
{
	texture->texture.Reset();
	texture->allocation.Reset();
}

void D3D12Device::destroyStagingTexture(D3D12StagingTexture* texture) noexcept
{
	unmapStagingTexture(texture);
	texture->buffer.Reset();
	texture->allocation.Reset();
}


void D3D12CommandList::clearTexture(RHITexture* texture, const RHIColor& color)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	endRendering();
	CHECK(texture && !isRHIDepthFormat(texture->getDesc().format), "clearTexture requires a color texture");
	validateRHITextureState(*texture, TransferDest, "clearTexture");

	if ((texture->getDesc().usage & RHITextureUsage::RenderTarget) == RHITextureUsage::None) {
		auto desc = texture->getDesc();
		desc.usage = RHITextureUsage::RenderTarget | RHITextureUsage::TransferSource | RHITextureUsage::TransferDest;
		auto temporary = device.createTexture(desc);
		transitionTexture(temporary.get(), temporary->getState(), TransferDest);
		clearTexture(temporary.get(), color);
		transitionTexture(temporary.get(), TransferDest, TransferSource);
		for (uint32 layer = 0; layer < desc.array_layers; ++layer)
			for (uint32 mip = 0; mip < desc.mip_levels; ++mip) {
				const auto slice = RHITextureSlice{}.setArrayLayer(layer).setMipLevel(mip);
				copyTexture(texture, slice, temporary.get(), slice);
			}
		return;
	}

	const auto state = texture->getState();
	transitionTexture(texture, state, RenderTarget);
	for (uint32 mip = 0; mip < texture->getDesc().mip_levels; ++mip) {
		auto view_desc = RHITextureViewDesc{}.setTexture(texture).setType(RHITextureViewType::RenderTarget).setSubresource(RHITextureSubresource{}.setBaseMipLevel(mip).setLevelCount(1));
		if (texture->getDesc().dimension == RHITextureDimension::TextureCube)
			view_desc.dimension = RHITextureViewDimension::Texture2DArray;
		auto view = device.createTextureView(view_desc);
		current_command->trackResource(view.get());
		current_command->getHandle()->ClearRenderTargetView(static_cast<D3D12TextureView*>(view.get())->getHandle(), &color.r, 0, nullptr);
	}
	transitionTexture(texture, RenderTarget, state);
}

void D3D12CommandList::clearDepthTexture(RHITexture* texture, bool clear_depth, float depth, bool clear_stencil, uint8 stencil)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	endRendering();
	CHECK(texture && isRHIDepthFormat(texture->getDesc().format), "clearDepthTexture requires a depth texture");
	if (!clear_depth && !clear_stencil)
		return;

	CHECK(!clear_stencil || hasRHIStencil(texture->getDesc().format), "Texture has no stencil plane");
	validateRHITextureState(*texture, TransferDest, "clearDepthTexture");

	const auto state = texture->getState();
	transitionTexture(texture, state, DepthWrite);
	for (uint32 mip = 0; mip < texture->getDesc().mip_levels; ++mip) {
		auto view_desc = RHITextureViewDesc{}.setTexture(texture).setType(RHITextureViewType::DepthStencil).setSubresource(RHITextureSubresource{}.setBaseMipLevel(mip).setLevelCount(1));
		if (texture->getDesc().dimension == RHITextureDimension::TextureCube)
			view_desc.dimension = RHITextureViewDimension::Texture2DArray;
		auto view = device.createTextureView(view_desc);
		current_command->trackResource(view.get());
		const auto flags = static_cast<D3D12_CLEAR_FLAGS>((clear_depth ? D3D12_CLEAR_FLAG_DEPTH : 0) | (clear_stencil ? D3D12_CLEAR_FLAG_STENCIL : 0));
		current_command->getHandle()->ClearDepthStencilView(static_cast<D3D12TextureView*>(view.get())->getHandle(), flags, depth, stencil, 0, nullptr);
	}
	transitionTexture(texture, DepthWrite, state);
}

void D3D12CommandList::copyTexture(RHITexture* dst, const RHITextureSlice& dst_slice, RHITexture* src, const RHITextureSlice& src_slice)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	endRendering();
	CHECK(dst && src && dst != src, "Texture copy requires two different resources");

	const auto d = normalizeRHITextureSlice(dst->getDesc(), dst_slice);
	const auto s = normalizeRHITextureSlice(src->getDesc(), src_slice);
	validateRHITextureCopy(dst->getDesc(), d, src->getDesc(), s);

	validateRHITextureState(*src, TransferSource, "copyTexture source");
	current_command->trackResource(src);
	validateRHITextureState(*dst, TransferDest, "copyTexture destination");
	current_command->trackResource(dst);

	const auto destination = textureLocation(static_cast<D3D12Texture*>(dst), d);
	const auto source = textureLocation(static_cast<D3D12Texture*>(src), s);
	const auto box = textureBox(s);
	const bool whole = isRHIDepthFormat(src->getDesc().format) || src->getDesc().sample_count > 1;
	current_command->getHandle()->CopyTextureRegion(&destination, d.x, d.y, d.z, &source, whole ? nullptr : &box);
}

void D3D12CommandList::copyTexture(RHIStagingTexture* dst, const RHITextureSlice& dst_slice, RHITexture* src, const RHITextureSlice& src_slice)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	endRendering();
	CHECK(dst && src, "Texture readback resources cannot be null");
	auto* staging = static_cast<D3D12StagingTexture*>(dst);
	CHECK(!staging->mapped && staging->access == RHIAccessMode::Read, "Texture readback requires an unmapped read staging texture");

	const auto d = normalizeRHITextureSlice(dst->getDesc(), dst_slice);
	const auto s = normalizeRHITextureSlice(src->getDesc(), src_slice);
	validateRHITextureCopy(dst->getDesc(), d, src->getDesc(), s);
	const bool depth = isRHIDepthFormat(src->getDesc().format);

	current_command->trackResource(staging);
	validateRHITextureState(*src, TransferSource, "copyTexture source");
	current_command->trackResource(src);

	D3D12_TEXTURE_COPY_LOCATION destination{};
	destination.pResource = staging->getHandle();
	destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	destination.PlacedFootprint = staging->footprints[d.mip_level + d.array_layer * staging->desc.mip_levels];
	const auto source = textureLocation(static_cast<D3D12Texture*>(src), s);
	const auto box = textureBox(s);
	current_command->getHandle()->CopyTextureRegion(&destination, d.x, d.y, d.z, &source, depth ? nullptr : &box);
}

void D3D12CommandList::copyTexture(RHITexture* dst, const RHITextureSlice& dst_slice, RHIStagingTexture* src, const RHITextureSlice& src_slice)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	endRendering();
	CHECK(dst && src, "Texture upload resources cannot be null");
	auto* staging = static_cast<D3D12StagingTexture*>(src);
	CHECK(!staging->mapped && staging->access == RHIAccessMode::Write, "Texture upload requires an unmapped write staging texture");
	const auto d = normalizeRHITextureSlice(dst->getDesc(), dst_slice);
	const auto s = normalizeRHITextureSlice(src->getDesc(), src_slice);
	validateRHITextureCopy(dst->getDesc(), d, src->getDesc(), s);
	const bool depth = isRHIDepthFormat(dst->getDesc().format);

	current_command->trackResource(staging);
	validateRHITextureState(*dst, TransferDest, "copyTexture destination");
	current_command->trackResource(dst);

	const auto destination = textureLocation(static_cast<D3D12Texture*>(dst), d);
	D3D12_TEXTURE_COPY_LOCATION source{};
	source.pResource = staging->getHandle();
	source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	source.PlacedFootprint = staging->footprints[s.mip_level + s.array_layer * staging->desc.mip_levels];
	const auto box = textureBox(s);
	current_command->getHandle()->CopyTextureRegion(&destination, d.x, d.y, d.z, &source, depth ? nullptr : &box);
}

void D3D12CommandList::writeTexture(RHITexture* texture, const RHITextureSlice& slice, const void* data, uint64 size)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	endRendering();
	if (size == 0)
		return;

	CHECK(texture && data, "Texture upload requires a texture and data");
	const auto& texture_desc = texture->getDesc();
	const auto region = normalizeRHITextureSlice(texture_desc, slice);
	const uint64 row_size = uint64(region.width) * getRHIFormatByteSize(texture_desc.format);
	CHECK(size >= row_size * region.height * region.depth, "Texture upload data is smaller than the requested slice");

	RHITextureDesc desc{};
	desc.setWidth(region.width)
	    .setHeight(region.height)
	    .setDepth(region.depth)
	    .setFormat(texture_desc.format)
	    .setDimension(texture_desc.dimension == RHITextureDimension::TextureCube ? RHITextureDimension::Texture2D : texture_desc.dimension)
	    .setUsage(RHITextureUsage::TransferSource);
	if (isRHIDepthFormat(desc.format))
		desc.usage |= RHITextureUsage::DepthStencil;

	auto staging = device.createStagingTexture(desc, RHIAccessMode::Write);
	uint64 row_pitch{}, depth_pitch{};
	auto* mapped = static_cast<std::byte*>(device.mapStagingTexture(staging.get(), RHIAccessMode::Write, row_pitch, depth_pitch));
	for (int z = 0; z < region.depth; ++z)
		for (int y = 0; y < region.height; ++y)
			std::memcpy(mapped + z * depth_pitch + y * row_pitch, static_cast<const std::byte*>(data) + (uint64(z) * region.height + y) * row_size, row_size);
	device.unmapStagingTexture(staging.get());
	transitionTexture(texture, texture->getState(), TransferDest);
	copyTexture(texture, region, staging.get(), {});
}

void D3D12CommandList::transitionTexture(RHITexture* resource, RHIResourceState before, RHIResourceState after)
{
	CHECK(recording && current_command, "D3D12 command list is not open");
	auto* texture = static_cast<D3D12Texture*>(resource);
	CHECK(texture && after != Unknown, "Invalid texture transition");
	CHECK(before == texture->state, "D3D12 texture '{}' state does not match the transition's before state", texture->getName());

	current_command->trackResource(texture);
	barrier(texture->getHandle(), toD3D12ResourceStates(texture->state), toD3D12ResourceStates(after));
	texture->state = after;
}

}        // namespace Vortex

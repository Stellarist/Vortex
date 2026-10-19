module Runtime.D3D12;

import d3d12;

namespace Vortex {

RHIRef<RHIFramebuffer> D3D12Device::createFramebuffer(const RHIFramebufferDesc& desc)
{
	validateRHIFramebufferDesc(desc);
	auto framebuffer = makeRHIRef<D3D12Framebuffer>(desc);

	framebuffer->color_attachments.reserve(desc.color_attachments.size());
	for (const auto& attachment : desc.color_attachments) {
		auto* texture_view = static_cast<D3D12TextureView*>(attachment.texture_view.get());
		framebuffer->color_attachments.push_back(texture_view->getHandle());
	}

	if (desc.depth_attachment.texture_view) {
		auto* texture_view = static_cast<D3D12TextureView*>(desc.depth_attachment.texture_view.get());
		framebuffer->depth_attachment = texture_view->getHandle();
	}

	return framebuffer;
}

}        // namespace Vortex

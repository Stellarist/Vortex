export module Runtime.D3D12:Framebuffer;

import d3d12;
import Core;
import Runtime.RHI;

export namespace Vortex {

class D3D12Framebuffer : public RHIFramebuffer {
private:
	RHIFramebufferDesc desc{};
	RHIFramebufferInfo info{};

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> color_attachments{};
	D3D12_CPU_DESCRIPTOR_HANDLE depth_attachment{};

	friend class D3D12Device;

public:
	D3D12Framebuffer(RHIFramebufferDesc desc) : desc(std::move(desc)), info(this->desc) {}

	const RHIFramebufferDesc& getDesc() const noexcept override { return desc; }
	const RHIFramebufferInfo& getFramebufferInfo() const noexcept override { return info; }

	const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& getColorAttachments() const noexcept { return color_attachments; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& getDepthAttachment() const noexcept { return depth_attachment; }
};

}        // namespace Vortex

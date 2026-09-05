export module Runtime.RDG:Pass;

import Core;
import :Resource;
import :Types;
import Runtime.RHI;

export namespace Vortex {

class RDGBuilder;
class RDGPassBuilder;
class RDGPassContext;

struct RDGPassDesc {
	std::string name{};
	RDGPassType type{RDGPassType::Raster};
	RDGPassFlags flags{RDGPassFlags::None};
	RDGPassAccess access{};

	std::optional<RDGRenderTargets> render_targets{};
};

struct RDGPassNode {
	RDGPassDesc desc{};
	RDGPassState state{};
	std::function<void(RDGPassContext&)> execute{};
};


class RDGPassContext {
private:
	RDGGraph* graph{};
	RHIDevice* device{};
	RHICommandList* command{};
	RHIFramebuffer* framebuffer{};

	const RDGPassDesc* desc{};

	RDGPassContext(RDGGraph& graph, RHIDevice& device, RHICommandList& command,
	    RHIFramebuffer* framebuffer, const RDGPassDesc& desc) noexcept;

	friend class RDGBuilder;

public:
	RHIDevice& getDevice() const noexcept;
	RHICommandList& getCommand() const noexcept;
	RHIFramebuffer& getFramebuffer() const;

	RHITexture& getTexture(RDGTextureRef texture) const;
	RHIBuffer& getBuffer(RDGBufferRef buffer) const;

	RHITextureView& getTextureView(RDGTextureViewRef view) const;
	RHIBufferView& getBufferView(RDGBufferViewRef view) const;
};


class RDGPassBuilder {
private:
	RDGGraph* graph{};
	RDGPassDesc* desc{};

	RDGPassBuilder(RDGGraph& graph, RDGPassDesc& desc) noexcept;

	void addAccess(RDGResource* resource, RDGAccess access, RHIResourceState state);
	void addAccess(RDGTextureViewRef view, RDGAccess access, RHIResourceState state);
	void addAccess(RDGBufferViewRef view, RDGAccess access, RHIResourceState state);

	friend class RDGBuilder;

public:
	RDGTextureViewRef createTextureView(std::string name, RDGTextureRef texture, const RHITextureViewDesc& desc = {});
	RDGBufferViewRef createBufferView(std::string name, RDGBufferRef buffer, const RHIBufferViewDesc& desc = {});

	void setColorAttachment(uint32 slot, RDGTextureRef texture, RHILoadOp load_op, RHIStoreOp store_op, const RHIClearValue& clear_value = {});
	void setDepthAttachment(RDGTextureRef texture, RHILoadOp load_op, RHIStoreOp store_op, const RHIClearValue& clear_value = {});
	void setDepthReadOnlyAttachment(RDGTextureRef texture);

	template <RDGAccessReference Ref>
	void read(Ref resource, RHIResourceState state)
	{
		addAccess(resource, RDGAccess::Read, state);
	}

	template <RDGAccessReference Ref>
	void write(Ref resource, RHIResourceState state)
	{
		addAccess(resource, RDGAccess::Write, state);
	}

	template <RDGAccessReference Ref>
	void readWrite(Ref resource, RHIResourceState state)
	{
		addAccess(resource, RDGAccess::ReadWrite, state);
	}
};

}        // namespace Vortex

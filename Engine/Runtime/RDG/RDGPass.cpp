module Runtime.RDG;

namespace Vortex {

static bool ownsResource(const RDGGraph& graph, const RDGResource* resource)
{
	if (!resource || resource->owner != &graph)
		return false;

	if (resource->type == RDGResourceType::Texture)
		return resource->index < graph.textures.size() && graph.textures[resource->index].get() == resource;
	else if (resource->type == RDGResourceType::Buffer)
		return resource->index < graph.buffers.size() && graph.buffers[resource->index].get() == resource;

	return false;
}

static bool ownsView(const RDGGraph& graph, RDGTextureViewRef view)
{
	return view && view->index < graph.texture_views.size() && graph.texture_views[view->index].get() == view;
}

static bool ownsView(const RDGGraph& graph, RDGBufferViewRef view)
{
	return view && view->index < graph.buffer_views.size() && graph.buffer_views[view->index].get() == view;
}

static bool declaresResource(const RDGPassDesc& desc, const RDGResource* resource)
{
	return std::ranges::any_of(desc.access.accesses, [resource](const RDGResourceAccess& access) {
		return access.resource == resource;
	});
}

static bool declaresView(const RDGPassDesc& desc, RDGTextureViewRef view)
{
	return std::ranges::find(desc.access.texture_views, view) != desc.access.texture_views.end();
}

static bool declaresView(const RDGPassDesc& desc, RDGBufferViewRef view)
{
	return std::ranges::find(desc.access.buffer_views, view) != desc.access.buffer_views.end();
}


RDGPass::RDGPass(RDGPassDesc declaration) :
    desc(std::move(declaration))
{
	if (desc.type == RDGPassType::Raster && !desc.render_targets)
		desc.render_targets.emplace();
}

RDGPassBuilder::RDGPassBuilder(RDGGraph& graph, RDGPassDesc& desc) noexcept :
    graph(&graph), desc(&desc)
{}

RDGTextureViewRef RDGPassBuilder::createTextureView(
    std::string name, RDGTextureRef texture, const RHITextureViewDesc& desc)
{
	CHECK(Argument, !name.empty(), "An RDG texture view must have a name");
	CHECK(Argument, ownsResource(*graph, texture), "An RDG texture view requires a texture owned by this graph");
	CHECK(Argument, !static_cast<bool>(desc.texture), "An RDG texture view descriptor cannot contain an RHI texture");

	auto view = std::make_unique<RDGTextureView>();
	view->index = static_cast<uint32>(graph->texture_views.size());
	view->name = std::move(name);
	view->texture = texture;
	view->desc = desc;
	auto* result = view.get();
	graph->texture_views.push_back(std::move(view));
	return result;
}

RDGBufferViewRef RDGPassBuilder::createBufferView(
    std::string name, RDGBufferRef buffer, const RHIBufferViewDesc& desc)
{
	CHECK(Argument, !name.empty(), "An RDG buffer view must have a name");
	CHECK(Argument, ownsResource(*graph, buffer), "An RDG buffer view requires a buffer owned by this graph");
	CHECK(Argument, !static_cast<bool>(desc.buffer), "An RDG buffer view descriptor cannot contain an RHI buffer");

	auto view = std::make_unique<RDGBufferView>();
	view->index = static_cast<uint32>(graph->buffer_views.size());
	view->name = std::move(name);
	view->buffer = buffer;
	view->desc = desc;
	auto* result = view.get();
	graph->buffer_views.push_back(std::move(view));
	return result;
}

void RDGPassBuilder::addAccess(RDGResource* resource, RDGAccess access, RHIResourceState state)
{
	CHECK(Argument, ownsResource(*graph, resource), "An RDG pass cannot use a resource owned by another graph");
	CHECK(Argument, state != Unknown, "An RDG resource access must have a known state");
	CHECK(Argument, !declaresResource(*desc, resource), "An RDG pass can only declare a resource once; use readWrite when needed");

	desc->access.accesses.push_back({resource, access, state});
}

void RDGPassBuilder::addAccess(RDGTextureViewRef view, RDGAccess access, RHIResourceState state)
{
	CHECK(Argument, ownsView(*graph, view), "An RDG pass cannot use a texture view owned by another graph");

	addAccess(view->texture, access, state);
	desc->access.texture_views.push_back(view);
}

void RDGPassBuilder::addAccess(RDGBufferViewRef view, RDGAccess access, RHIResourceState state)
{
	CHECK(Argument, ownsView(*graph, view), "An RDG pass cannot use a buffer view owned by another graph");

	addAccess(view->buffer, access, state);
	desc->access.buffer_views.push_back(view);
}

void RDGPassBuilder::setColorAttachment(uint32 slot, RDGTextureRef texture, RHILoadOp load_op, RHIStoreOp store_op, const RHIClearValue& clear_value)
{
	CHECK(desc->type == RDGPassType::Raster, "Only a raster RDG pass can declare color attachments");
	CHECK(Argument, slot == desc->render_targets->colors.size(), "RDG color attachment slots must be contiguous and declared in order");

	addAccess(texture, load_op == RHILoadOp::Load ? RDGAccess::ReadWrite : RDGAccess::Write, RenderTarget);
	desc->render_targets->colors.push_back({texture, load_op, store_op, clear_value});
}

void RDGPassBuilder::setDepthAttachment(RDGTextureRef texture, RHILoadOp load_op, RHIStoreOp store_op, const RHIClearValue& clear_value)
{
	CHECK(desc->type == RDGPassType::Raster, "Only a raster RDG pass can declare a depth attachment");
	CHECK(!desc->render_targets->depth.has_value(), "An RDG raster pass can only declare one depth attachment");

	addAccess(texture, load_op == RHILoadOp::Load ? RDGAccess::ReadWrite : RDGAccess::Write, DepthWrite);
	desc->render_targets->depth = RDGAttachment{texture, load_op, store_op, clear_value, false};
}

void RDGPassBuilder::setDepthReadOnlyAttachment(RDGTextureRef texture)
{
	CHECK(desc->type == RDGPassType::Raster, "Only a raster RDG pass can declare a read-only depth attachment");
	CHECK(!desc->render_targets->depth.has_value(), "An RDG raster pass can only declare one depth attachment");

	addAccess(texture, RDGAccess::Read, DepthRead);
	desc->render_targets->depth = RDGAttachment{texture, RHILoadOp::Load, RHIStoreOp::Store, {}, true};
}


RDGPassContext::RDGPassContext(RDGGraph& graph, RHIDevice& device,
    RHICommandList& command, RHIFramebuffer* framebuffer, const RDGPassDesc& desc) noexcept :
    graph(&graph), desc(&desc), device(&device), command(&command), framebuffer(framebuffer)
{}

RHIDevice& RDGPassContext::getDevice() const noexcept
{
	return *device;
}

RHICommandList& RDGPassContext::getCommand() const noexcept
{
	return *command;
}

RHIFramebuffer& RDGPassContext::getFramebuffer() const
{
	CHECK(framebuffer, "The executing RDG pass has no raster framebuffer");
	return *framebuffer;
}

RHITexture& RDGPassContext::getTexture(RDGTextureRef texture) const
{
	CHECK(ownsResource(*graph, texture), "The RDG texture is not owned by the executing graph");
	CHECK(declaresResource(*desc, texture), "The executing RDG pass did not declare this texture");
	CHECK(texture->texture, "The RDG texture has not been allocated");

	return *texture->texture;
}

RHIBuffer& RDGPassContext::getBuffer(RDGBufferRef buffer) const
{
	CHECK(ownsResource(*graph, buffer), "The RDG buffer is not owned by the executing graph");
	CHECK(declaresResource(*desc, buffer), "The executing RDG pass did not declare this buffer");
	CHECK(buffer->buffer, "The RDG buffer has not been allocated");

	return *buffer->buffer;
}

RHITextureView& RDGPassContext::getTextureView(RDGTextureViewRef view) const
{
	CHECK(ownsView(*graph, view), "The RDG texture view is not owned by the executing graph");
	CHECK(declaresView(*desc, view), "The executing RDG pass did not declare this texture view");
	CHECK(view->view, "The RDG texture view has not been created");

	return *view->view;
}

RHIBufferView& RDGPassContext::getBufferView(RDGBufferViewRef view) const
{
	CHECK(ownsView(*graph, view), "The RDG buffer view is not owned by the executing graph");
	CHECK(declaresView(*desc, view), "The executing RDG pass did not declare this buffer view");
	CHECK(view->view, "The RDG buffer view has not been created");

	return *view->view;
}

}        // namespace Vortex

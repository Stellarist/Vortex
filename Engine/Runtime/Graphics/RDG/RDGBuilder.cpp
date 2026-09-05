module Runtime.Graphics;

namespace Vortex {

static void sortDependencies(std::vector<RDGPassHandle>& dependencies)
{
	std::ranges::sort(dependencies, [](RDGPassHandle lhs, RDGPassHandle rhs) {
		return lhs.index < rhs.index;
	});
	dependencies.erase(std::unique(dependencies.begin(), dependencies.end()), dependencies.end());
}

static RHIRef<RHIFramebuffer> createFramebuffer(RHIDevice& device, const RDGPassNode& pass)
{
	if (!pass.pass->getDesc().render_targets || (pass.pass->getDesc().render_targets->colors.empty() && !pass.pass->getDesc().render_targets->depth))
		return {};

	auto* first_texture = pass.pass->getDesc().render_targets->colors.empty() ?
	    pass.pass->getDesc().render_targets->depth->texture :
	    pass.pass->getDesc().render_targets->colors.front().texture;

	RHIFramebufferDesc framebuffer_desc{};
	framebuffer_desc.setWidth(first_texture->desc.width)
	    .setHeight(first_texture->desc.height)
	    .setSampleCount(first_texture->desc.sample_count);

	auto requireTexture = [&](RDGTextureRef texture) -> RDGTexture& {
		CHECK(texture->texture, "RDG raster pass '{}' uses an unallocated attachment '{}'", pass.pass->getName(), texture->name);
		return *texture;
	};

	for (const auto& attachment : pass.pass->getDesc().render_targets->colors) {
		auto& texture = requireTexture(attachment.texture);
		auto view = device.createTextureView(RHITextureViewDesc{}
		        .setTexture(texture.texture.get())
		        .setType(RHITextureViewType::RenderTarget));

		framebuffer_desc.addColorAttachment(RHIFramebufferAttachment{}
		        .setTextureView(view.get())
		        .setLoadOp(attachment.load_op)
		        .setStoreOp(attachment.store_op)
		        .setClearValue(attachment.clear_value)
		        .setReadOnly(attachment.read_only));
	}

	if (pass.pass->getDesc().render_targets->depth) {
		const auto& attachment = *pass.pass->getDesc().render_targets->depth;

		auto& texture = requireTexture(attachment.texture);
		auto view = device.createTextureView(RHITextureViewDesc{}
		        .setTexture(texture.texture.get())
		        .setType(RHITextureViewType::DepthStencil));

		framebuffer_desc.setDepthAttachment(RHIFramebufferAttachment{}
		        .setTextureView(view.get())
		        .setLoadOp(attachment.load_op)
		        .setStoreOp(attachment.store_op)
		        .setClearValue(attachment.clear_value)
		        .setReadOnly(attachment.read_only));
	}

	return device.createFramebuffer(framebuffer_desc);
}


void RDGBuilder::addDependency(RDGPassNode& pass, RDGPassHandle dependency)
{
	if (dependency.valid())
		pass.state.dependencies.push_back(dependency);
}

void RDGBuilder::applyBarriers(RHICommandList& command, std::span<const RDGBarrier> barriers)
{
	for (const auto& barrier : barriers) {
		if (barrier.resource->type == RDGResourceType::Texture) {
			auto* texture = static_cast<RDGTexture*>(barrier.resource);
			command.transitionTexture(texture->texture.get(), barrier.before, barrier.after);
		} else {
			auto* buffer = static_cast<RDGBuffer*>(barrier.resource);
			command.transitionBuffer(buffer->buffer.get(), barrier.before, barrier.after);
		}
	}
}

void RDGBuilder::createPassViews(RHIDevice& device, const RDGPassNode& pass)
{
	for (auto* view : pass.pass->getDesc().access.texture_views) {
		if (view->view)
			continue;

		auto desc = view->desc;
		desc.setTexture(view->texture->texture.get());
		view->view = device.createTextureView(desc);
		view->view->setName(view->name);
	}

	for (auto* view : pass.pass->getDesc().access.buffer_views) {
		if (view->view)
			continue;

		auto desc = view->desc;
		desc.setBuffer(view->buffer->buffer.get());
		view->view = device.createBufferView(desc);
		view->view->setName(view->name);
	}
}

RDGTextureRef RDGBuilder::createTexture(std::string name, const RHITextureDesc& desc)
{
	CHECK(!executed, "Cannot create an RDG texture after execution");
	validateRDGName(name, "texture");
	validateRHITextureDesc(desc);

	auto texture = std::make_unique<RDGTexture>();
	texture->owner = &graph;
	texture->index = static_cast<uint32>(graph.textures.size());
	texture->name = std::move(name);
	texture->type = RDGResourceType::Texture;
	texture->desc = desc;

	auto* result = texture.get();
	graph.textures.push_back(std::move(texture));
	compiled = false;
	return result;
}

RDGBufferRef RDGBuilder::createBuffer(std::string name, const RHIBufferDesc& desc)
{
	CHECK(!executed, "Cannot create an RDG buffer after execution");
	validateRDGName(name, "buffer");
	validateRHIBufferDesc(desc);

	auto buffer = std::make_unique<RDGBuffer>();
	buffer->owner = &graph;
	buffer->index = static_cast<uint32>(graph.buffers.size());
	buffer->name = std::move(name);
	buffer->type = RDGResourceType::Buffer;
	buffer->desc = desc;

	auto* result = buffer.get();
	graph.buffers.push_back(std::move(buffer));
	compiled = false;
	return result;
}

RDGTextureRef RDGBuilder::registerExternalTexture(std::string name, RHITexture& texture, RHIResourceState initial_state, RHIResourceState final_state)
{
	auto* resource = createTexture(std::move(name), texture.getDesc());
	resource->texture = &texture;
	resource->initial_state = initial_state;
	resource->final_state = final_state;
	resource->external = true;
	return resource;
}

RDGBufferRef RDGBuilder::registerExternalBuffer(std::string name, RHIBuffer& buffer, RHIResourceState initial_state, RHIResourceState final_state)
{
	auto* resource = createBuffer(std::move(name), buffer.getDesc());
	resource->buffer = &buffer;
	resource->initial_state = initial_state;
	resource->final_state = final_state;
	resource->external = true;
	return resource;
}

void RDGBuilder::addOutput(RDGTextureRef texture)
{
	const bool owned = texture &&
	    texture->owner == &graph &&
	    texture->type == RDGResourceType::Texture &&
	    texture->index < graph.textures.size() &&
	    graph.textures[texture->index].get() == texture;
	CHECK(Argument, owned, "Cannot mark a texture owned by another graph as output");

	texture->output = true;
	compiled = false;
}

void RDGBuilder::addOutput(RDGBufferRef buffer)
{
	const bool owned = buffer &&
	    buffer->owner == &graph &&
	    buffer->type == RDGResourceType::Buffer &&
	    buffer->index < graph.buffers.size() &&
	    graph.buffers[buffer->index].get() == buffer;
	CHECK(Argument, owned, "Cannot mark a buffer owned by another graph as output");

	buffer->output = true;
	compiled = false;
}

void RDGBuilder::buildDependencies()
{
	struct ResourceTracker {
		RDGPassHandle last_writer{};
		std::vector<RDGPassHandle> active_readers{};
		bool content_valid{};
	};

	std::vector<ResourceTracker> texture_trackers(graph.textures.size());
	for (const auto& texture : graph.textures)
		texture_trackers[texture->index].content_valid = texture->external;

	std::vector<ResourceTracker> buffer_trackers(graph.buffers.size());
	for (const auto& buffer : graph.buffers)
		buffer_trackers[buffer->index].content_valid = buffer->external;

	for (uint32 pass_index = 0; pass_index < graph.passes.size(); ++pass_index) {
		auto& pass = graph.passes[pass_index];
		const auto& desc = pass.pass->getDesc();
		pass.state.dependencies.clear();

		const RDGPassHandle current_pass{pass_index};
		for (const auto& access : desc.access.accesses) {
			const auto& resource = *access.resource;
			auto& tracker = resource.type == RDGResourceType::Texture ?
			    texture_trackers[resource.index] :
			    buffer_trackers[resource.index];

			if (reads(access.access)) {
				CHECK(Logic, tracker.content_valid,
				    "RDG pass '{}' reads resource '{}' with undefined contents",
				    pass.pass->getName(), resource.name);
				addDependency(pass, tracker.last_writer);
			}

			if (!writes(access.access)) {
				tracker.active_readers.push_back(current_pass);
				continue;
			}

			addDependency(pass, tracker.last_writer);
			for (auto reader : tracker.active_readers)
				addDependency(pass, reader);

			tracker.active_readers.clear();
			tracker.last_writer = current_pass;
			tracker.content_valid = true;
		}

		sortDependencies(pass.state.dependencies);
		if (!desc.render_targets)
			continue;

		for (const auto& attachment : desc.render_targets->colors) {
			if (attachment.store_op != RHIStoreOp::Discard)
				continue;
			texture_trackers[attachment.texture->index].content_valid = false;
		}

		const auto& depth = desc.render_targets->depth;
		if (!depth || depth->store_op != RHIStoreOp::Discard)
			continue;
		texture_trackers[depth->texture->index].content_valid = false;
	}
}

void RDGBuilder::calculateCulling()
{
	std::vector<RDGPassHandle> roots;
	std::vector<RDGPassHandle> texture_last_writers(graph.textures.size());
	std::vector<RDGPassHandle> buffer_last_writers(graph.buffers.size());

	for (auto& pass : graph.passes)
		pass.state.culled = true;

	for (uint32 pass_index = 0; pass_index < graph.passes.size(); ++pass_index) {
		const auto& pass = graph.passes[pass_index];
		const RDGPassHandle handle{pass_index};
		if (hasAnyFlags(pass.pass->getFlags(), RDGPassFlags::NeverCull))
			roots.push_back(handle);

		for (const auto& access : pass.pass->getDesc().access.accesses) {
			if (!writes(access.access))
				continue;

			const auto& resource = *access.resource;
			if (resource.external)
				roots.push_back(handle);

			if (access.resource->type == RDGResourceType::Texture)
				texture_last_writers[access.resource->index] = handle;
			else
				buffer_last_writers[access.resource->index] = handle;
		}
	}

	for (uint32 index = 0; index < graph.textures.size(); ++index) {
		const auto& resource = *graph.textures[index];
		if (!resource.output)
			continue;

		CHECK(texture_last_writers[index].valid(), "RDG output texture '{}' is never written", resource.name);
		roots.push_back(texture_last_writers[index]);
	}

	for (uint32 index = 0; index < graph.buffers.size(); ++index) {
		const auto& resource = *graph.buffers[index];
		if (!resource.output)
			continue;

		CHECK(buffer_last_writers[index].valid(), "RDG output buffer '{}' is never written", resource.name);
		roots.push_back(buffer_last_writers[index]);
	}

	while (!roots.empty()) {
		const auto handle = roots.back();
		roots.pop_back();

		auto& pass = graph.passes[handle.index];
		if (!pass.state.culled)
			continue;

		pass.state.culled = false;
		for (auto dependency : pass.state.dependencies)
			roots.push_back(dependency);
	}
}

void RDGBuilder::calculateLifetimes()
{
	for (auto& texture : graph.textures) {
		texture->first_use.reset();
		texture->last_use.reset();
	}

	for (auto& buffer : graph.buffers) {
		buffer->first_use.reset();
		buffer->last_use.reset();
	}

	execution_order.clear();
	for (uint32 pass_index = 0; pass_index < graph.passes.size(); ++pass_index) {
		const auto& pass = graph.passes[pass_index];
		if (pass.state.culled)
			continue;

		const auto execution_index = static_cast<uint32>(execution_order.size());
		execution_order.push_back({pass_index});

		for (const auto& access : pass.pass->getDesc().access.accesses) {
			auto& resource = *access.resource;
			if (!resource.first_use)
				resource.first_use = execution_index;
			resource.last_use = execution_index;
		}
	}
}

void RDGBuilder::buildBarrierPlan()
{
	struct ResourceBarrierState {
		RHIResourceState state{Unknown};
		std::optional<RDGAccess> last_access;
	};

	std::vector<ResourceBarrierState> texture_states;
	texture_states.reserve(graph.textures.size());
	for (const auto& texture : graph.textures)
		texture_states.push_back({texture->initial_state});

	std::vector<ResourceBarrierState> buffer_states;
	buffer_states.reserve(graph.buffers.size());
	for (const auto& buffer : graph.buffers)
		buffer_states.push_back({buffer->initial_state});

	for (auto handle : execution_order) {
		auto& pass = graph.passes[handle.index];
		pass.state.barriers.clear();

		for (const auto& access : pass.pass->getDesc().access.accesses) {
			auto& current = access.resource->type == RDGResourceType::Texture ?
			    texture_states[access.resource->index] :
			    buffer_states[access.resource->index];

			const bool access_hazard = current.last_access &&
			    (writes(*current.last_access) || writes(access.access));
			if (current.state != access.state || access_hazard)
				pass.state.barriers.push_back({access.resource, current.state, access.state});

			current.state = access.state;
			current.last_access = access.access;
		}
	}

	epilogue_barriers.clear();
	for (uint32 index = 0; index < graph.textures.size(); ++index) {
		const auto& resource = *graph.textures[index];
		if (!resource.external || !resource.first_use || resource.final_state == Unknown)
			continue;

		const auto& current = texture_states[index];
		if (current.state != resource.final_state || (current.last_access && writes(*current.last_access)))
			epilogue_barriers.push_back({graph.textures[index].get(), current.state, resource.final_state});
	}

	for (uint32 index = 0; index < graph.buffers.size(); ++index) {
		const auto& resource = *graph.buffers[index];
		if (!resource.external || !resource.first_use || resource.final_state == Unknown)
			continue;

		const auto& current = buffer_states[index];
		if (current.state != resource.final_state || (current.last_access && writes(*current.last_access)))
			epilogue_barriers.push_back({graph.buffers[index].get(), current.state, resource.final_state});
	}
}

void RDGBuilder::allocateResources(RHIDevice& device)
{
	for (auto& texture : graph.textures) {
		if (texture->external || !texture->first_use)
			continue;

		texture->texture = device.createTexture(texture->desc);
		CHECK(texture->texture, "Failed to allocate RDG texture '{}'", texture->name);
		texture->texture->setName(texture->name);
		texture->initial_state = texture->texture->getState();
	}

	for (auto& buffer : graph.buffers) {
		if (buffer->external || !buffer->first_use)
			continue;

		buffer->buffer = device.createBuffer(buffer->desc);
		CHECK(buffer->buffer, "Failed to allocate RDG buffer '{}'", buffer->name);
		buffer->buffer->setName(buffer->name);
		buffer->initial_state = buffer->buffer->getState();
	}
}

void RDGBuilder::compile()
{
	if (compiled)
		return;
	validateRDGGraph(graph);

	buildDependencies();
	calculateCulling();
	calculateLifetimes();
	compiled = true;
}

void RDGBuilder::execute(RHIDevice& device, RHICommandList& command)
{
	CHECK(!executed, "An RDG can only execute once");

	compile();
	executed = true;

	allocateResources(device);
	buildBarrierPlan();

	for (auto handle : execution_order) {
		auto& pass = graph.passes[handle.index];
		applyBarriers(command, pass.state.barriers);

		auto framebuffer = pass.pass->getType() == RDGPassType::Raster ?
		    createFramebuffer(device, pass) :
		    RHIRef<RHIFramebuffer>{};
		if (framebuffer)
			framebuffer->setName(pass.pass->getName());

		RDGPassContext context(graph, device, command, framebuffer.get(), pass.pass->getDesc());
		createPassViews(device, pass);

		command.beginDebugLabel(pass.pass->getName());
		if (framebuffer)
			command.beginRendering(framebuffer.get());
		pass.pass->execute(context);
		if (framebuffer)
			command.endRendering();
		command.endDebugLabel();
	}

	applyBarriers(command, epilogue_barriers);
}

RDGPassHandle RDGBuilder::addPass(std::unique_ptr<RDGPass> pass)
{
	CHECK(!executed, "Cannot add a pass after the RDG has executed");
	CHECK(Argument, pass, "Cannot add an empty RDG pass");
	validateRDGName(pass->getName(), "pass");

	const auto pass_count = graph.passes.size();
	graph.passes.push_back({std::move(pass)});

	compiled = false;
	auto& added = graph.passes.back();

	RDGPassBuilder builder(graph, added.pass->desc);
	added.pass->setup(builder);

	return {static_cast<uint32>(pass_count)};
}

}        // namespace Vortex

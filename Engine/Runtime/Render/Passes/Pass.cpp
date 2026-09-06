module Runtime.Render;

namespace Vortex {

void submitDrawList(RHICommandList& command, const DrawList& draws,
    const RHIGraphicsState& base_state, RHIBindingSet* scene_binding,
    bool bind_material, RHIBindingSet* trailing_binding)
{
	for (const auto& draw : draws.getDraws()) {
		auto state = base_state;
		if (scene_binding)
			state.addBindingSet(scene_binding);
		if (bind_material && draw.material_binding)
			state.addBindingSet(draw.material_binding.get());
		if (draw.object_binding)
			state.addBindingSet(draw.object_binding.get());
		state.addVertexBuffer(draw.vertex_buffer)
		    .setIndexBuffer(draw.index_buffer);
		if (trailing_binding)
			state.addBindingSet(trailing_binding);

		command.setGraphicsState(state);
		command.drawIndexed(draw.arguments);
	}
}

}        // namespace Vortex

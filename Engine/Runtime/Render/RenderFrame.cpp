module Runtime.Render;

namespace Vortex {

RenderView::RenderView(const RenderViewDesc& desc) :
    camera_id(desc.camera_id),
    view(desc.view),
    projection(desc.projection),
    position(desc.position),
    has_camera(desc.has_camera)
{}

DrawList::DrawList(DrawListDesc desc) :
    type(desc.type),
    draws(std::move(desc.draws))
{}

RenderFrame::RenderFrame(RenderView render_view, DrawList opaque_draws,
    DrawList transparent_draws, DrawList shadow_draws,
    RHIBindingLayout* scene_binding_layout,
    RHIBindingLayout* material_binding_layout,
    RHIBindingLayout* object_binding_layout,
    RHIBindingSet* scene_binding_set,
    RHIBufferView* scene_constant_view,
    RHIBufferView* scene_light_view,
    std::optional<Vec3> directional_light_direction,
    RenderFrameStats frame_stats) :
    view(std::move(render_view)),
    opaque(std::move(opaque_draws)),
    transparent(std::move(transparent_draws)),
    shadow(std::move(shadow_draws)),
    scene_layout(scene_binding_layout),
    material_layout(material_binding_layout),
    object_layout(object_binding_layout),
    scene_binding(scene_binding_set),
    scene_constants(scene_constant_view),
    scene_lights(scene_light_view),
    main_directional_light_direction(std::move(directional_light_direction)),
    stats(frame_stats)
{}

}        // namespace Vortex

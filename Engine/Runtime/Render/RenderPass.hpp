export module Runtime.Render:Pass;

import Core;
import Runtime.RHI;
import Runtime.RDG;
import :Core;

namespace Vortex {

class RenderManager;

RDGBindingSet makeViewBindings(RDGPassBuilder&, RenderViewRefs, RHIBindingLayout&, std::string_view name);
RDGBindingSet makeShadowBindings(RDGPassBuilder&, ShadowMap, RHIBindingLayout&, RHISampler&, std::string_view name);
RHIViewportState passViewport(const RHIFramebufferDesc&);
void drawFrameList(RHICommandList&, std::span<const RenderDraw>, const RHIGraphicsState&,
    RHIBindingSet* view, RHIBindingSet* trailing = nullptr);
RHIInputLayout& meshInputLayout(RenderManager&);
RHIBindingLayout& viewBindingLayout(RenderManager&);
RHIBindingLayout& shadowSampleLayout(RenderManager&);
RHISampler& screenSampler(RenderManager&);

}        // namespace Vortex

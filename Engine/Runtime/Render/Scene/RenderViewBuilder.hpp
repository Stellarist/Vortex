export module Runtime.Render:Scene.ViewBuilder;

import Core;
import :Frame;
import :Scene;

export namespace Vortex {

class RenderViewBuilder {
public:
	RenderFrame build(RenderScene& scene, const RenderViewDesc& view,
	    bool enable_frustum_culling) const;
};

}        // namespace Vortex

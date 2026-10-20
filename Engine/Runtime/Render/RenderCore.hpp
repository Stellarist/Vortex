export module Runtime.Render:Core;

import Core;
import Runtime.RHI;
import Runtime.RDG;

export namespace Vortex {

struct RenderSettings {
	bool frustum_culling{true};
	bool directional_shadows{true};
};

struct RenderStats {
	uint64 frame_index{};
	uint32 rdg_pass_count{};
	uint32 draw_count{};
	uint32 opaque_draw_count{};
	uint32 masked_draw_count{};
	uint32 transparent_draw_count{};
	uint32 shadow_draw_count{};
	uint32 culled_draw_count{};
	bool frame_rendered{};
};

struct RenderView {
	uint64 id{};
	Mat4 view{1.0f};
	Mat4 projection{1.0f};
	Vec3 position{0.0f};
	bool valid{};
	RHIExtent extent{};
};

struct RenderViewRefs {
	RDGBufferRef constants{};
	RDGBufferRef lights{};
};

struct ShadowMap {
	RDGTextureRef texture{};
	RDGBufferRef constants{};
};

struct GBuffer {
	RDGTextureRef albedo{};
	RDGTextureRef normal{};
	RDGTextureRef emissive{};
	RDGTextureRef depth{};
};

struct RenderDraw {
	struct PrimitiveConstants {
		Mat4 model{1.0f};
		Mat4 normal_matrix{1.0f};
	};

	RHIVertexBufferBinding vertices{};
	RHIIndexBufferBinding indices{};
	RHIRef<RHIBindingSet> material{};
	PrimitiveConstants transform{};
	RHIDrawArguments arguments{};
};

struct RenderCameraConstants {
	Mat4 view{1.0f};
	Mat4 projection{1.0f};
	Mat4 inverse_view_projection{1.0f};
	Vec4 position{0.0f, 0.0f, 0.0f, 1.0f};
};

struct RenderViewConstants {
	RenderCameraConstants camera{};
	Vec4 ambient_color{0.1f, 0.1f, 0.1f, 1.0f};
	uint32 light_count{};
	uint32 reserved[2]{};
	uint32 viewport_width{};
	uint32 viewport_height{};
	uint32 padding[3]{};
};

struct RenderLightConstants {
	Vec4 position{};
	Vec4 direction{};
	Vec4 color{};
	Vec4 params{};
};

}        // namespace Vortex

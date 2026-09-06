export module Runtime.Render:Settings;

import Core;

export namespace Vortex {

enum class RenderPathType : uint8 {
	Forward,
	Deferred,
};

struct RendererStats {
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

struct RenderSettings {
	RenderPathType render_path{RenderPathType::Forward};

	bool frustum_culling{true};
	bool directional_shadows{true};
	float shadow_bias{0.0015f};

	static RenderSettings load(const std::filesystem::path& path);
};

}        // namespace Vortex

export module Runtime.Render:Frame;

import Core;
import Runtime.RHI;

export namespace Vortex {

enum class DrawListType : uint8 {
	Opaque,
	Transparent,
	Shadow,
};

struct RenderViewDesc {
	uint64 camera_id{};
	Mat4 view{1.0f};
	Mat4 projection{1.0f};
	Vec3 position{0.0f};
	bool has_camera{};
};

class RenderView {
private:
	uint64 camera_id{};
	Mat4 view{1.0f};
	Mat4 projection{1.0f};
	Vec3 position{0.0f};
	bool has_camera{};

public:
	RenderView() = default;
	RenderView(const RenderViewDesc& desc);

	uint64 getCameraId() const noexcept { return camera_id; }
	const Mat4& getViewMatrix() const noexcept { return view; }
	const Mat4& getProjectionMatrix() const noexcept { return projection; }
	const Vec3& getPosition() const noexcept { return position; }
	bool hasCamera() const noexcept { return has_camera; }
};

struct DrawPacket {
	RHIVertexBufferBinding vertex_buffer{};
	RHIIndexBufferBinding index_buffer{};
	RHIRef<RHIBindingSet> material_binding{};
	RHIRef<RHIBindingSet> object_binding{};
	RHIDrawArguments arguments{};
};

struct DrawListDesc {
	DrawListType type{DrawListType::Opaque};
	std::vector<DrawPacket> draws{};
};

struct RenderFrameStats {
	size_t opaque_draw_count{};
	size_t masked_draw_count{};
	size_t transparent_draw_count{};
	size_t shadow_draw_count{};
	size_t culled_draw_count{};
};

class DrawList {
private:
	DrawListType type{DrawListType::Opaque};
	std::vector<DrawPacket> draws{};

public:
	DrawList() = default;
	DrawList(DrawListDesc desc);

	DrawListType getType() const noexcept { return type; }
	size_t getDrawCount() const noexcept { return draws.size(); }
	std::span<const DrawPacket> getDraws() const noexcept { return draws; }
};

class RenderFrame {
private:
	RenderView view;
	DrawList opaque;
	DrawList transparent;
	DrawList shadow;

	RHIRef<RHIBindingLayout> scene_layout;
	RHIRef<RHIBindingLayout> material_layout;
	RHIRef<RHIBindingLayout> object_layout;
	RHIRef<RHIBindingSet> scene_binding;
	RHIRef<RHIBufferView> scene_constants;
	RHIRef<RHIBufferView> scene_lights;

	std::optional<Vec3> main_directional_light_direction;
	RenderFrameStats stats{};

public:
	RenderFrame() = default;
	RenderFrame(RenderView render_view, DrawList opaque_draws,
	    DrawList transparent_draws, DrawList shadow_draws,
	    RHIBindingLayout* scene_binding_layout,
	    RHIBindingLayout* material_binding_layout,
	    RHIBindingLayout* object_binding_layout,
	    RHIBindingSet* scene_binding_set,
	    RHIBufferView* scene_constant_view,
	    RHIBufferView* scene_light_view,
	    std::optional<Vec3> directional_light_direction = {},
	    RenderFrameStats frame_stats = {});

	const RenderView& getView() const noexcept { return view; }
	const DrawList& getOpaqueList() const noexcept { return opaque; }
	const DrawList& getTransparentList() const noexcept { return transparent; }
	const DrawList& getShadowList() const noexcept { return shadow; }

	RHIBindingLayout* getSceneLayout() const noexcept { return scene_layout.get(); }
	RHIBindingLayout* getMaterialLayout() const noexcept { return material_layout.get(); }
	RHIBindingLayout* getObjectLayout() const noexcept { return object_layout.get(); }
	RHIBindingSet* getSceneBinding() const noexcept { return scene_binding.get(); }
	RHIBufferView* getSceneConstants() const noexcept { return scene_constants.get(); }
	RHIBufferView* getSceneLights() const noexcept { return scene_lights.get(); }

	bool valid() const noexcept
	{
		return scene_layout && material_layout && object_layout && scene_binding;
	}
	const std::optional<Vec3>& getMainDirectionalLightDirection() const noexcept
	{
		return main_directional_light_direction;
	}
	const RenderFrameStats& getStats() const noexcept { return stats; }
};

}        // namespace Vortex

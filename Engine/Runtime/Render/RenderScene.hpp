export module Runtime.Render:Scene;

import Core;
import Runtime.World;
import Runtime.RHI;
import :Core;

namespace Vortex {

class RenderManager;

struct RenderLight {
	uint64 id{};
	uint32 type{};
	Vec3 position{0.0f};
	Vec3 direction{0.0f, -1.0f, 0.0f};
	Vec3 color{1.0f};
	float intensity{1.0f};
	float range{};
	float inner_cone{};
	float outer_cone{};
};

struct RenderMaterialData {
	uint64 id{};
	uint64 revision{};
	std::string name{};
	Vec4 base_color{1.0f};
	float metallic{};
	float roughness{1.0f};
	float alpha_cutoff{0.5f};
	MaterialAsset::AlphaMode alpha_mode{};
	Vec3 emissive{0.0f};
	std::array<AssetHandle<TextureAsset>, static_cast<size_t>(MaterialTextureSlot::Count)> textures{};
};

struct RenderSurface {
	MeshSection section{};
	uint64 material_id{};
};

struct RenderPrimitive {
	uint64 id{};
	uint64 mesh_id{};
	std::vector<RenderSurface> surfaces{};
	Mat4 model{1.0f};
	Bounds bounds{};
	bool casts_shadow{};
};

struct RenderScene {
	std::vector<RenderLight> lights{};
	std::vector<RenderPrimitive> primitives{};
	std::unordered_map<uint64, AssetHandle<MeshAsset>> meshes{};
	std::unordered_map<uint64, RenderMaterialData> materials{};
};

RenderScene collectRenderScene(const World& world);

struct RenderPreparedView {
	RenderView view{};
	RenderViewConstants constants{};
	std::vector<RenderLightConstants> lights{};
	std::optional<Vec3> shadow_direction{};
	std::vector<RenderDraw> opaque{};
	std::vector<RenderDraw> transparent{};
	std::vector<RenderDraw> shadow{};
	RenderStats stats{};
};

RenderView collectRenderView(const World& world, RHIExtent extent);
RenderPreparedView prepareRenderView(const RenderScene& scene, const RenderManager& resources,
    const RenderView& view, const RenderSettings& settings);

}        // namespace Vortex

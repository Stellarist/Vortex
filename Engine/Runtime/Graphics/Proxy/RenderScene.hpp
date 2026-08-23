export module Runtime.Graphics:RenderScene;

import Core;
import Runtime.Asset;
import Runtime.World;
import :RenderResources;
import :RHICommand;

export namespace Vortex {

class RenderScene {
private:
	struct MeshDraw {
		RenderMesh* mesh{};
		uint32      section_index{};
	};

	// Set 0: Scene-level bindings
	RHIRef<RHIBindingLayout> scene_layout;
	RHIRef<RHIBuffer>        scene_constant_buffer;
	RHIRef<RHIBufferView>    scene_constant_buffer_view;
	RHIRef<RHIBindingSet>    scene_binding_set;
	RenderSceneData          scene_data;

	// Set 1: Material-level
	RHIRef<RHIBindingLayout>                     material_layout;
	RHIRef<RHISampler>                           material_sampler;
	std::vector<std::unique_ptr<RenderMaterial>> render_materials;
	std::vector<std::unique_ptr<RenderTexture>>  render_textures;

	std::unordered_map<uint64, RenderTexture*>  render_texture_map;
	std::unordered_map<uint64, RenderMaterial*> render_material_map;

	// Set 2: Object-level
	RHIRef<RHIBindingLayout>                 object_layout;
	std::vector<std::unique_ptr<RenderMesh>> render_meshes;

	std::unordered_map<const MeshComponent*, RenderMesh*> render_mesh_map;
	std::unordered_map<uint64, std::vector<MeshDraw>>     meshes_by_material;

	size_t asset_state{};

	const World* world{};
	RHIContext*  context{};

	void createSceneLayouts();
	void createSceneBindingSet();

	void updateCamera();
	void updateLights();
	void updateMeshes();

	void collectAssets(std::vector<MeshComponent*>& meshes,
	    std::vector<AssetHandle<MaterialAsset>>&    materials,
	    std::vector<AssetHandle<TextureAsset>>&     textures) const;
	void loadTextures(const std::vector<AssetHandle<TextureAsset>>& textures);
	void loadMaterials(const std::vector<AssetHandle<MaterialAsset>>& materials);
	void loadMeshes(const std::vector<MeshComponent*>& meshes);
	void sortMeshes(const std::vector<MeshComponent*>& meshes);

	size_t calculateAssetState() const;
	bool   needsRebuild() const;
	void   clear();

public:
	RenderScene(RHIContext& context, const World& world);

	void update(float dt);
	void rebuild();
	void draw(RHICommandList& command, const RHIGraphicsState& base_state);

	RHIBindingSet*    getSceneBindingSet() { return scene_binding_set.get(); }
	RHIBindingLayout* getSceneLayout() { return scene_layout.get(); }
	RHIBindingLayout* getMaterialLayout() { return material_layout.get(); }
	RHIBindingLayout* getObjectLayout() { return object_layout.get(); }

	const World* getWorld() const { return world; }
};

}        // namespace Vortex

export module Runtime.Graphics:RenderScene;

import Core;
import Runtime.World;
import :RenderResources;
import :RHICommand;

export namespace Vortex {

class RenderScene {
private:
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

	std::unordered_map<std::shared_ptr<Texture>, RenderTexture*> render_texture_map;

	// Set 2: Object-level
	RHIRef<RHIBindingLayout>                 object_layout;
	std::vector<std::unique_ptr<RenderMesh>> render_meshes;

	std::unordered_map<std::shared_ptr<Material>, std::vector<RenderMesh*>> meshes_by_material;
	std::unordered_map<std::shared_ptr<SubMesh>, RenderMesh*>               render_mesh_map;

	size_t last_submesh_count{0};
	size_t last_texture_count{0};
	size_t last_material_count{0};

	const World* world{};

	RHIContext* context{};

	void createSceneLayouts();
	void createSceneBindingSet();

	void updateCamera();
	void updateLights();
	void updateMesh();

	void loadTextures();
	void loadMaterials();
	void loadMeshes();
	void sortMeshes();

	bool needsRebuild() const;
	void clear();

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

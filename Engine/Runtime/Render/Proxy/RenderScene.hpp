#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "RenderResources.hpp"
#include "Runtime/Render/RHI/RHICommand.hpp"
#include "Runtime/World/World.hpp"
#include "Runtime/World/Resources/Texture.hpp"

class RenderScene {
private:
	// Set 0: Scene-level descriptor
	std::unique_ptr<RHIDescriptorLayout> scene_layout;
	std::unique_ptr<RHIDescriptorSet>    scene_descriptor;
	std::unique_ptr<RHIBuffer>           scene_uniform;
	RenderSceneData                      scene_data;

	// Set 1: Material-level
	std::unique_ptr<RHIDescriptorLayout>         material_layout;
	std::shared_ptr<RHISampler>                  material_sampler;
	std::vector<std::unique_ptr<RenderMaterial>> render_materials;
	std::vector<std::unique_ptr<RenderTexture>>  render_textures;

	std::unordered_map<std::shared_ptr<Texture>, RenderTexture*> render_texture_map;

	// Set 2: Object-level
	std::unique_ptr<RHIDescriptorLayout>     object_layout;
	std::vector<std::unique_ptr<RenderMesh>> render_meshes;

	std::unordered_map<std::shared_ptr<Material>, std::vector<RenderMesh*>> meshes_by_material;
	std::unordered_map<std::shared_ptr<SubMesh>, RenderMesh*>               render_mesh_map;

	size_t last_submesh_count{0};
	size_t last_texture_count{0};
	size_t last_material_count{0};

	const World* world{};

	RHIContext* context{};

	void createSceneLayouts();
	void createSceneDescriptor();

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

	RHIDescriptorSet*    getSceneDescriptor() { return scene_descriptor.get(); }
	RHIDescriptorLayout* getSceneLayout() { return scene_layout.get(); }
	RHIDescriptorLayout* getMaterialLayout() { return material_layout.get(); }
	RHIDescriptorLayout* getObjectLayout() { return object_layout.get(); }

	const World* getWorld() const { return world; }
};

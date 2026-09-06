export module Runtime.Render:Scene;

import Core;
import Runtime.RHI;
import Runtime.World;
import :Frame;
import :Scene.Resource;
import :Scene.Proxy;

export namespace Vortex {

class RenderViewBuilder;

class RenderScene {
private:
	friend class RenderViewBuilder;

	RHIRef<RHIBindingLayout> scene_layout;
	RHIRef<RHIBuffer> scene_constant_buffer;
	RHIRef<RHIBufferView> scene_constant_buffer_view;
	RHIRef<RHIBuffer> light_buffer;
	RHIRef<RHIBufferView> light_buffer_view;
	RHIRef<RHIBindingSet> scene_binding_set;
	std::unordered_map<uint64, LightProxy> light_proxies;
	std::vector<uint64> light_order;
	std::unordered_map<uint64, std::unique_ptr<MeshProxy>> mesh_proxies;
	uint64 light_topology_revision{1};
	uint32 light_count{};
	size_t uploaded_light_count{};

	std::unique_ptr<RenderResourceCache> resources;
	RHIContext* context{};

	void createSceneLayouts();
	void createSceneBindingSet();
	void synchronizeLights(const World& world);
	void synchronizeMeshes(const World& world);
	void updateLights();
	void updateView(const RenderViewDesc& view);
	const LightProxy* getMainDirectionalLight() const noexcept;

public:
	RenderScene(RHIContext& context);

	void update(const World& world);

	RHIBuffer* getLightBuffer() { return light_buffer.get(); }
	uint32 getLightCount() const noexcept { return light_count; }
	uint64 getLightTopologyRevision() const noexcept { return light_topology_revision; }
	size_t getPrimitiveCount() const noexcept { return mesh_proxies.size(); }
	size_t getMeshResourceCount() const noexcept { return resources->getMeshCount(); }
};

}        // namespace Vortex

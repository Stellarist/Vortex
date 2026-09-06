module Runtime.Render;

namespace Vortex {

namespace {

inline constexpr uint32 MAX_LIGHTS = 1024;

struct CameraConstants {
	Mat4 view{1.0f};
	Mat4 projection{1.0f};
	Mat4 inverse_view_projection{1.0f};
	Vec4 position{0.0f, 0.0f, 0.0f, 1.0f};
};

struct SceneConstants {
	CameraConstants camera;
	Vec4 ambient_color{0.1f, 0.1f, 0.1f, 1.0f};
	uint32 light_count{};
	uint32 reserved[2]{};
	uint32 viewport_width{};
	uint32 viewport_height{};
	uint32 padding[3]{};
};

static_assert(sizeof(CameraConstants) == 208);
static_assert(sizeof(SceneConstants) == 256);

}        // namespace

RenderScene::RenderScene(RHIContext& new_context) :
    context(&new_context)
{
	createSceneLayouts();
	createSceneBindingSet();
}

void RenderScene::createSceneLayouts()
{
	auto& device = context->getDevice();
	scene_layout = device.createBindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::constantBuffer(0))
	        .addItem(RHIBindingLayoutItem::structuredBufferSRV(1)));
	scene_layout->setName("Render.Scene.BindingLayout");
	auto material_layout = device.createBindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::constantBuffer(0))
	        .addItem(RHIBindingLayoutItem::textureSRV(1))
	        .addItem(RHIBindingLayoutItem::textureSRV(2))
	        .addItem(RHIBindingLayoutItem::sampler(3)));
	material_layout->setName("Render.Material.BindingLayout");
	auto object_layout = device.createBindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::constantBuffer(0)));
	object_layout->setName("Render.Object.BindingLayout");
	resources = std::make_unique<RenderResourceCache>(
	    device, *material_layout, *object_layout);
}

void RenderScene::createSceneBindingSet()
{
	RHIBufferDesc constant_buffer_desc{};
	constant_buffer_desc.setSize(sizeof(SceneConstants))
	    .setUsage(RHIBufferUsage::ConstantBuffer | RHIBufferUsage::CopyDest)
	    .setAccess(RHIAccessMode::Write);
	scene_constant_buffer = context->getDevice().createBuffer(constant_buffer_desc);
	scene_constant_buffer->setName("Scene.Constants");
	scene_constant_buffer_view = context->getDevice().createBufferView(
	    RHIBufferViewDesc{}.setBuffer(scene_constant_buffer.get()).setType(RHIBufferViewType::Constant));
	scene_constant_buffer_view->setName("Scene.Constants.CBV");

	light_buffer = context->getDevice().createBuffer(RHIBufferDesc{}
	        .setSize(static_cast<uint64>(MAX_LIGHTS) * LightProxy::constantSize())
	        .setStride(LightProxy::constantSize())
	        .setUsage(RHIBufferUsage::StorageBuffer)
	        .setAccess(RHIAccessMode::Write));
	light_buffer->setName("Scene.Lights");
	light_buffer_view = context->getDevice().createBufferView(RHIBufferViewDesc{}
	        .setBuffer(light_buffer.get())
	        .setType(RHIBufferViewType::Structured));
	light_buffer_view->setName("Scene.Lights.SRV");

	scene_binding_set = context->getDevice().createBindingSet(RHIBindingSetDesc{}
	                                                              .addItem(RHIBindingSetItem::constantBuffer(0, scene_constant_buffer_view.get()))
	                                                              .addItem(RHIBindingSetItem::structuredBufferSRV(1, light_buffer_view.get())),
	    *scene_layout);
	scene_binding_set->setName("Scene.BindingSet");
}

void RenderScene::synchronizeLights(const World& world)
{
	const auto lights = world.getComponents<LightComponent>();
	std::unordered_set<uint64> active;
	active.reserve(lights.size());
	std::vector<uint64> next_order;
	next_order.reserve(lights.size());
	for (const auto* light : lights) {
		if (!light || !light->isEnabled() || !light->getOwner() ||
		    !light->getOwner()->isEnabled())
			continue;
		const auto id = light->getUid();
		if (!active.insert(id).second)
			continue;
		next_order.push_back(id);
		const auto existing = light_proxies.find(id);
		if (existing != light_proxies.end())
			existing->second.update(*light);
		else
			light_proxies.emplace(id, LightProxy(*light));
	}
	std::erase_if(light_proxies, [&active](const auto& entry) {
		return !active.contains(entry.first);
	});
	if (next_order != light_order) {
		light_order = std::move(next_order);
		if (++light_topology_revision == 0)
			++light_topology_revision;
	}
}

void RenderScene::synchronizeMeshes(const World& world)
{
	std::unordered_set<uint64> active;
	const auto components = world.getComponents<MeshComponent>();
	active.reserve(components.size());
	for (const auto* component : components) {
		if (!component || !component->getMesh() || !component->getMesh()->valid())
			continue;
		const auto id = component->getUid();
		active.insert(id);
		auto* mesh = resources->findMesh(component->getMesh());
		CHECK(mesh, "Mesh proxy resource was not loaded");
		auto proxy = mesh_proxies.find(id);
		if (proxy == mesh_proxies.end()) {
			auto created = std::make_unique<MeshProxy>(
			    context->getDevice(), id, *mesh, *resources->getObjectLayout());
			proxy = mesh_proxies.emplace(id, std::move(created)).first;
		}
		proxy->second->update(context->getDevice(), *component, *mesh, *resources);
	}
	std::erase_if(mesh_proxies, [&active](const auto& entry) {
		return !active.contains(entry.first);
	});
}

void RenderScene::updateLights()
{
	void* mapped = context->getDevice().mapBuffer(light_buffer.get(), RHIAccessMode::Write);
	CHECK(mapped, "Failed to map the scene light buffer");
	auto* output = static_cast<std::byte*>(mapped);
	size_t count = 0;
	for (const auto id : light_order) {
		if (count >= MAX_LIGHTS)
			break;
		const auto proxy = light_proxies.find(id);
		if (proxy == light_proxies.end())
			continue;
		proxy->second.writeConstants(
		    output + count++ * LightProxy::constantSize());
	}
	light_count = static_cast<uint32>(count);

	if (uploaded_light_count > count)
		std::memset(output + count * LightProxy::constantSize(), 0,
		    (uploaded_light_count - count) * LightProxy::constantSize());
	uploaded_light_count = count;
	context->getDevice().unmapBuffer(light_buffer.get());
}

void RenderScene::updateView(const RenderViewDesc& view)
{
	SceneConstants constants{};
	const auto extent = context->getExtent();
	constants.light_count = light_count;
	constants.viewport_width = extent.width;
	constants.viewport_height = extent.height;
	if (view.has_camera) {
		constants.camera.view = view.view;
		constants.camera.projection = view.projection;
		constants.camera.position = Vec4(view.position, 1.0f);
		constants.camera.inverse_view_projection = Math::inverse(
		    view.projection * view.view);
	}

	void* mapped = context->getDevice().mapBuffer(
	    scene_constant_buffer.get(), RHIAccessMode::Write);
	CHECK(mapped, "Failed to map the scene constant buffer");
	std::memcpy(mapped, &constants, sizeof(constants));
	context->getDevice().unmapBuffer(scene_constant_buffer.get());
}

void RenderScene::update(const World& world)
{
	resources->synchronize(world);
	synchronizeLights(world);
	synchronizeMeshes(world);
	updateLights();
}

const LightProxy* RenderScene::getMainDirectionalLight() const noexcept
{
	for (const auto id : light_order) {
		const auto proxy = light_proxies.find(id);
		if (proxy != light_proxies.end() &&
		    proxy->second.getType() == LightProxy::Type::Directional)
			return &proxy->second;
	}
	return nullptr;
}

}        // namespace Vortex

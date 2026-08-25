module;

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <cassert>
#include <tiny_gltf.h>

module Editor;

namespace Vortex {

static constexpr std::array attributes_names = {
    "POSITION",
    "NORMAL",
    "TEXCOORD_0",
    "COLOR_0",
};

static std::string makeAssetName(std::string_view source_name, std::string_view type, std::string_view virtual_path)
{
	if (!source_name.empty())
		return std::string(source_name);

	const auto separator = virtual_path.find_last_of('/');
	const auto index = separator == std::string_view::npos ? virtual_path : virtual_path.substr(separator + 1);
	return index.empty() ? std::string(type) : std::format("{}_{}", type, index);
}

static void applyNodeTransform(Transform& transform, const tinygltf::Node& tfnode)
{
	if (const auto& translation = tfnode.translation; !translation.empty())
		transform.setTranslation({static_cast<float>(translation[0]),
		    static_cast<float>(translation[1]),
		    static_cast<float>(translation[2])});

	if (const auto& rotation = tfnode.rotation; !rotation.empty())
		transform.setRotation({static_cast<float>(rotation[3]),
		    static_cast<float>(rotation[0]),
		    static_cast<float>(rotation[1]),
		    static_cast<float>(rotation[2])});

	if (const auto& scale = tfnode.scale; !scale.empty())
		transform.setScaling({static_cast<float>(scale[0]),
		    static_cast<float>(scale[1]),
		    static_cast<float>(scale[2])});

	if (const auto& matrix = tfnode.matrix; !matrix.empty())
		transform.setMatrix({static_cast<float>(matrix[0]), static_cast<float>(matrix[1]), static_cast<float>(matrix[2]), static_cast<float>(matrix[3]), static_cast<float>(matrix[4]), static_cast<float>(matrix[5]), static_cast<float>(matrix[6]), static_cast<float>(matrix[7]), static_cast<float>(matrix[8]), static_cast<float>(matrix[9]), static_cast<float>(matrix[10]), static_cast<float>(matrix[11]), static_cast<float>(matrix[12]), static_cast<float>(matrix[13]), static_cast<float>(matrix[14]), static_cast<float>(matrix[15])});
}

std::unique_ptr<Scene> AssetImporter::loadScene(AssetManager& assets, std::string_view scene_path)
{
	// Load Scene
	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        error, warn;
	if (!loader.LoadASCIIFromFile(&model, &error, &warn, scene_path.data())) {
		if (!error.empty())
			throw std::runtime_error("Error: " + error);
		if (!warn.empty())
			throw std::runtime_error("Warning: " + warn);
		throw std::runtime_error("Failed to load glTF file");
	}

	auto scene = std::make_unique<Scene>();
	scene->setName("Default Scene");
	const auto source_path = std::filesystem::path(scene_path).lexically_normal().generic_string();

	// Load Textures
	std::vector<AssetHandle<TextureAsset>> textures;
	textures.reserve(model.textures.size());
	for (size_t index = 0; index < model.textures.size(); ++index) {
		auto virtual_path = std::format("{}#texture/{}", source_path, index);
		auto texture = assets.findByPath<TextureAsset>(virtual_path);
		if (!texture)
			texture = assets.add(parseTextureAsset(model.textures[index], model, std::move(virtual_path)));
		textures.push_back(std::move(texture));
	}

	auto default_base_color = getDefaultBaseColorTexture(assets);
	auto default_metallic_roughness = getDefaultMetallicRoughnessTexture(assets);

	// Load Materials
	std::vector<AssetHandle<MaterialAsset>> materials;
	materials.reserve(model.materials.size());
	for (size_t index = 0; index < model.materials.size(); ++index) {
		auto virtual_path = std::format("{}#material/{}", source_path, index);
		auto material = assets.findByPath<MaterialAsset>(virtual_path);
		if (!material) {
			auto parsed_material = parseMaterialAsset(model.materials[index],
			    model,
			    textures,
			    default_base_color,
			    default_metallic_roughness,
			    std::move(virtual_path));
			material = assets.add(std::move(parsed_material));
		}
		materials.push_back(std::move(material));
	}

	auto default_material = getDefaultMaterial(assets);

	// Load mesh assets. Multiple glTF nodes can reference the same MeshAsset.
	std::vector<AssetHandle<MeshAsset>> meshes;
	meshes.reserve(model.meshes.size());
	for (size_t mesh_index = 0; mesh_index < model.meshes.size(); ++mesh_index) {
		auto virtual_path = std::format("{}#mesh/{}", source_path, mesh_index);
		auto mesh = assets.findByPath<MeshAsset>(virtual_path);
		if (!mesh) {
			auto parsed_mesh = parseMeshAsset(model.meshes[mesh_index], model, materials, default_material, std::move(virtual_path));
			mesh = assets.add(std::move(parsed_mesh));
		}
		meshes.push_back(std::move(mesh));
	}

	// Load Actors and their owned components.
	std::vector<std::unique_ptr<Actor>> actors;
	for (size_t index = 0; index < model.nodes.size(); index++) {
		const auto& tfnode = model.nodes[index];
		auto        actor = parseActor(tfnode);

		if (tfnode.mesh >= 0) {
			assert(tfnode.mesh < model.meshes.size());
			auto mesh = parseMeshComponent(model.meshes[tfnode.mesh]);
			mesh->setMesh(meshes[tfnode.mesh]);
			actor->addComponent(std::move(mesh));
		}

		if (tfnode.camera >= 0) {
			assert(tfnode.camera < model.cameras.size());
			actor->addComponent(parseCameraComponent(model.cameras[tfnode.camera]));
		}

		if (tfnode.light >= 0) {
			assert(tfnode.light < model.lights.size());
			actor->addComponent(parseLightComponent(model.lights[tfnode.light]));
		}

		if (!actor->hasRootComponent())
			actor->addComponent<SceneComponent>("RootSceneComponent");
		applyNodeTransform(actor->getRootComponent()->getTransform(), tfnode);

		actors.push_back(std::move(actor));
	}

	// Build the selected glTF scene hierarchy. Nodes that belong only to other
	// glTF scenes are not registered in this runtime Scene.
	std::queue<std::pair<Actor&, int>> traverse_actors;

	if (model.scenes.empty())
		throw std::runtime_error("No default scene found in glTF file");
	const size_t scene_index = model.defaultScene >= 0 && model.defaultScene < model.scenes.size() ? static_cast<size_t>(model.defaultScene) : 0;
	const auto&  tfscene = model.scenes[scene_index];

	auto root_actor = std::make_unique<Actor>(tfscene.name);
	root_actor->addComponent<SceneComponent>("RootSceneComponent");
	std::vector<bool> active_nodes(model.nodes.size(), false);
	for (auto node_index : tfscene.nodes)
		traverse_actors.push({std::ref(*root_actor), node_index});

	while (!traverse_actors.empty()) {
		auto actor_it = traverse_actors.front();
		traverse_actors.pop();
		if (actor_it.second < 0 || actor_it.second >= actors.size())
			continue;
		if (active_nodes[actor_it.second])
			continue;
		active_nodes[actor_it.second] = true;

		auto& current_actor = *actors.at(actor_it.second);
		auto& parent_actor = actor_it.first;
		parent_actor.attachActor(current_actor);

		for (auto child_node_index : model.nodes.at(actor_it.second).children)
			traverse_actors.push({std::ref(current_actor), child_node_index});
	}

	std::vector<std::unique_ptr<Actor>> scene_actors;
	scene_actors.push_back(std::move(root_actor));
	for (size_t index = 0; index < actors.size(); ++index)
		if (active_nodes[index])
			scene_actors.push_back(std::move(actors[index]));
	scene->setActors(std::move(scene_actors));

	initDefaultCameraComponent(*scene);
	initDefaultLightComponent(*scene);
	initDefaultCameraControllerComponent(*scene);

	return scene;
}

std::unique_ptr<Actor> AssetImporter::parseActor(const tinygltf::Node& tfnode)
{
	return std::make_unique<Actor>(tfnode.name);
}

std::unique_ptr<MeshComponent> AssetImporter::parseMeshComponent(const tinygltf::Mesh& tfmesh)
{
	auto mesh = std::make_unique<MeshComponent>(tfmesh.name);

	return mesh;
}

std::unique_ptr<CameraComponent> AssetImporter::parseCameraComponent(const tinygltf::Camera& tfcamera)
{
	auto camera = std::make_unique<PerspectiveCameraComponent>(tfcamera.name);
	camera->setAspectRatio(static_cast<float>(tfcamera.perspective.aspectRatio));
	camera->setFov(static_cast<float>(tfcamera.perspective.yfov));
	camera->setNearPlane(static_cast<float>(tfcamera.perspective.znear));
	camera->setFarPlane(static_cast<float>(tfcamera.perspective.zfar));

	return camera;
}

std::unique_ptr<LightComponent> AssetImporter::parseLightComponent(const tinygltf::Light& tflight)
{
	std::unique_ptr<LightComponent> light{};
	if (tflight.type == "directional") {
		light = std::make_unique<DirectionalLightComponent>(tflight.name);

	} else if (tflight.type == "point") {
		light = std::make_unique<PointLightComponent>(tflight.name);
		dynamic_cast<PointLightComponent*>(light.get())->setRange(static_cast<float>(tflight.range));

	} else if (tflight.type == "spot") {
		light = std::make_unique<SpotLightComponent>(tflight.name);
		dynamic_cast<SpotLightComponent*>(light.get())->setRange(static_cast<float>(tflight.range));
		dynamic_cast<SpotLightComponent*>(light.get())->setInnerConeAngle(static_cast<float>(tflight.spot.innerConeAngle));
		dynamic_cast<SpotLightComponent*>(light.get())->setOuterConeAngle(static_cast<float>(tflight.spot.outerConeAngle));

	} else
		throw std::runtime_error("Unknown light type");

	light->setColor({static_cast<float>(tflight.color[0]),
	    static_cast<float>(tflight.color[1]),
	    static_cast<float>(tflight.color[2])});
	light->setIntensity(static_cast<float>(tflight.intensity));

	return light;
}

std::shared_ptr<MeshAsset> AssetImporter::parseMeshAsset(const tinygltf::Mesh& tfmesh,
    const tinygltf::Model&                                                     tfmodel,
    const std::vector<AssetHandle<MaterialAsset>>&                             materials,
    AssetHandle<MaterialAsset>                                                 default_material,
    std::string                                                                virtual_path)
{
	auto mesh = std::make_shared<MeshAsset>(makeAssetName(tfmesh.name, "Mesh", virtual_path), std::move(virtual_path));

	std::vector<Vertex>                     vertices;
	std::vector<uint32>                     indices;
	std::vector<MeshSection>                sections;
	std::vector<AssetHandle<MaterialAsset>> mesh_materials;

	for (const auto& tfprimitive : tfmesh.primitives) {
		auto position_it = tfprimitive.attributes.find("POSITION");
		if (position_it == tfprimitive.attributes.end())
			continue;

		MeshSection section{};
		section.first_vertex = static_cast<uint32>(vertices.size());
		section.vertex_count = getAttributeCount(&tfmodel, position_it->second);

		std::vector<Vertex> section_vertices(section.vertex_count);
		const auto&         position_data = getAttributeDataView(tfmodel, position_it->second);
		const auto*         position_ptr = reinterpret_cast<const float*>(position_data.data());
		for (uint32 index = 0; index < section.vertex_count; ++index)
			section_vertices[index].pos = Vec3(position_ptr[index * 3 + 0], position_ptr[index * 3 + 1], position_ptr[index * 3 + 2]);

		if (auto normal_it = tfprimitive.attributes.find("NORMAL"); normal_it != tfprimitive.attributes.end()) {
			const auto& normal_data = getAttributeDataView(tfmodel, normal_it->second);
			const auto* normal_ptr = reinterpret_cast<const float*>(normal_data.data());
			for (uint32 index = 0; index < section.vertex_count; ++index)
				section_vertices[index].normal = Vec3(normal_ptr[index * 3 + 0], normal_ptr[index * 3 + 1], normal_ptr[index * 3 + 2]);
		}

		if (auto uv_it = tfprimitive.attributes.find("TEXCOORD_0"); uv_it != tfprimitive.attributes.end()) {
			const auto& uv_data = getAttributeDataView(tfmodel, uv_it->second);
			const auto* uv_ptr = reinterpret_cast<const float*>(uv_data.data());
			for (uint32 index = 0; index < section.vertex_count; ++index)
				section_vertices[index].uv = Vec2(uv_ptr[index * 2 + 0], uv_ptr[index * 2 + 1]);
		}

		if (auto color_it = tfprimitive.attributes.find("COLOR_0"); color_it != tfprimitive.attributes.end()) {
			const auto& color_data = getAttributeDataView(tfmodel, color_it->second);
			const auto* color_ptr = reinterpret_cast<const float*>(color_data.data());
			for (uint32 index = 0; index < section.vertex_count; ++index)
				section_vertices[index].color = Vec4(color_ptr[index * 4 + 0], color_ptr[index * 4 + 1], color_ptr[index * 4 + 2], color_ptr[index * 4 + 3]);
		}

		vertices.insert(vertices.end(), section_vertices.begin(), section_vertices.end());
		section.first_index = static_cast<uint32>(indices.size());

		std::vector<uint32> section_indices;
		if (tfprimitive.indices >= 0) {
			auto indices_raw_data = getAttributeDataView(tfmodel, tfprimitive.indices);
			auto index_byte_size = getAttributeSize(&tfmodel, tfprimitive.indices);

			switch (index_byte_size) {
			case 1:
				section_indices = convertData<uint8, uint32>(indices_raw_data);
				break;
			case 2:
				section_indices = convertData<uint16, uint32>(indices_raw_data);
				break;
			case 4:
				section_indices = convertData<uint32, uint32>(indices_raw_data);
				break;
			default:
				throw std::runtime_error("Unsupported index byte size");
			}
		} else {
			section_indices.resize(section.vertex_count);
			std::iota(section_indices.begin(), section_indices.end(), 0U);
		}

		section.index_count = static_cast<uint32>(section_indices.size());
		indices.insert(indices.end(), section_indices.begin(), section_indices.end());

		section.material_slot = static_cast<uint32>(mesh_materials.size());
		if (tfprimitive.material >= 0 && static_cast<size_t>(tfprimitive.material) < materials.size())
			mesh_materials.push_back(materials[tfprimitive.material]);
		else
			mesh_materials.push_back(default_material);
		sections.push_back(section);
	}

	mesh->setVertices(std::move(vertices));
	mesh->setIndices(std::move(indices));
	mesh->setSections(std::move(sections));
	mesh->setMaterials(std::move(mesh_materials));
	return mesh;
}

std::shared_ptr<TextureAsset> AssetImporter::parseTextureAsset(const tinygltf::Texture& tftexture,
    const tinygltf::Model&                                                              tfmodel,
    std::string                                                                         virtual_path)
{
	auto texture = std::make_shared<TextureAsset>(
	    makeAssetName(tftexture.name, "Texture", virtual_path), TextureAsset::Dimension::Tex2D, std::move(virtual_path));

	if (tftexture.source < 0 || tftexture.source >= static_cast<int>(tfmodel.images.size()))
		return texture;

	const auto& tfimage = tfmodel.images[tftexture.source];
	if (tfimage.width == 0 || tfimage.height == 0 || tfimage.image.empty())
		return texture;
	else if (tfimage.component != 3 && tfimage.component != 4)
		throw std::runtime_error("Unsupported image component count");

	texture->setWidth(tfimage.width);
	texture->setHeight(tfimage.height);
	texture->setFormat(4);

	std::vector<uint8> rgba_data;
	if (tfimage.component == 4)
		rgba_data = tfimage.image;
	else if (tfimage.component == 3) {
		rgba_data.resize(tfimage.width * tfimage.height * 4);
		for (size_t i = 0, j = 0; i < tfimage.image.size(); i += 3, j += 4) {
			rgba_data[j + 0] = tfimage.image[i + 0];
			rgba_data[j + 1] = tfimage.image[i + 1];
			rgba_data[j + 2] = tfimage.image[i + 2];
			rgba_data[j + 3] = 255;
		}
	}

	texture->setData(std::move(rgba_data));

	return texture;
}

std::shared_ptr<MaterialAsset> AssetImporter::parseMaterialAsset(const tinygltf::Material& tfmaterial,
    const tinygltf::Model&                                                                 tfmodel,
    const std::vector<AssetHandle<TextureAsset>>&                                          textures,
    AssetHandle<TextureAsset>                                                              default_base_color,
    AssetHandle<TextureAsset>                                                              default_metallic_roughness,
    std::string                                                                            virtual_path)
{
	auto material = std::make_shared<MaterialAsset>(
	    makeAssetName(tfmaterial.name, "Material", virtual_path), MaterialAsset::ShadingModel::Lit, std::move(virtual_path));

	const auto& pbr = tfmaterial.pbrMetallicRoughness;
	material->setAlbedo(
	    {static_cast<float>(pbr.baseColorFactor[0]),
	        static_cast<float>(pbr.baseColorFactor[1]),
	        static_cast<float>(pbr.baseColorFactor[2]),
	        static_cast<float>(pbr.baseColorFactor[3])});

	material->setMetallic(static_cast<float>(pbr.metallicFactor));
	material->setRoughness(static_cast<float>(pbr.roughnessFactor));

	if (pbr.baseColorTexture.index >= 0 && static_cast<size_t>(pbr.baseColorTexture.index) < textures.size())
		material->setTexture("baseColor", textures[pbr.baseColorTexture.index]);
	else
		material->setTexture("baseColor", std::move(default_base_color));

	if (pbr.metallicRoughnessTexture.index >= 0 && static_cast<size_t>(pbr.metallicRoughnessTexture.index) < textures.size())
		material->setTexture("metallicRoughness", textures[pbr.metallicRoughnessTexture.index]);
	else
		material->setTexture("metallicRoughness", std::move(default_metallic_roughness));

	material->setEmissive({static_cast<float>(tfmaterial.emissiveFactor[0]),
	    static_cast<float>(tfmaterial.emissiveFactor[1]),
	    static_cast<float>(tfmaterial.emissiveFactor[2])});

	if (tfmaterial.alphaMode == "BLEND")
		material->setAlphaMode(MaterialAsset::AlphaMode::Blend);
	else if (tfmaterial.alphaMode == "OPAQUE")
		material->setAlphaMode(MaterialAsset::AlphaMode::Opaque);
	else if (tfmaterial.alphaMode == "MASK")
		material->setAlphaMode(MaterialAsset::AlphaMode::Mask);

	material->setAlphaCutoff(static_cast<float>(tfmaterial.alphaCutoff));
	material->setDoubleSided(tfmaterial.doubleSided);

	return material;
}

std::unique_ptr<CameraComponent> AssetImporter::createDefaultCameraComponent(const std::string& name)
{
	auto camera = std::make_unique<PerspectiveCameraComponent>(name);
	camera->setAspectRatio(16.0f / 9.0f);
	camera->setFov(Math::radians(45.0f));
	camera->setNearPlane(0.1f);
	camera->setFarPlane(1000.0f);

	return camera;
}

std::unique_ptr<LightComponent> AssetImporter::createDefaultLightComponent(const std::string& name)
{
	auto light = std::make_unique<DirectionalLightComponent>(name);
	light->setColor({1.0f, 1.0f, 1.0f});
	light->setIntensity(1.0f);

	return light;
}

std::shared_ptr<TextureAsset> AssetImporter::createDefaultTextureAsset(std::string name, std::string virtual_path)
{
	auto texture = std::make_shared<TextureAsset>(std::move(name), TextureAsset::Dimension::Tex2D, std::move(virtual_path));
	texture->setWidth(1);
	texture->setHeight(1);
	texture->setFormat(4);
	texture->setData({255, 255, 255, 255});

	return texture;
}

std::shared_ptr<MaterialAsset> AssetImporter::createDefaultMaterialAsset(std::string name, std::string virtual_path)
{
	auto material = std::make_shared<MaterialAsset>(std::move(name), MaterialAsset::ShadingModel::Lit, std::move(virtual_path));
	material->setAlbedo({1.0f, 1.0f, 1.0f, 1.0f});
	material->setMetallic(0.0f);
	material->setRoughness(1.0f);
	material->setAlphaMode(MaterialAsset::AlphaMode::Opaque);
	material->setDoubleSided(false);

	return material;
}

std::unique_ptr<CameraControllerComponent> AssetImporter::createDefaultCameraControllerComponent(const std::string& name)
{
	auto controller = std::make_unique<CameraControllerComponent>(name);

	return controller;
}

void AssetImporter::initDefaultCameraComponent(Scene& scene)
{
	if (scene.hasComponent<CameraComponent>())
		return;

	auto default_camera = createDefaultCameraComponent();
	auto camera_actor = std::make_unique<Actor>("DefaultCameraActor");
	camera_actor->addComponent(std::move(default_camera));
	scene.addActor(std::move(camera_actor));
}

void AssetImporter::initDefaultLightComponent(Scene& scene)
{
	if (scene.hasComponent<LightComponent>())
		return;

	auto default_light = createDefaultLightComponent();
	auto light_actor = std::make_unique<Actor>("DefaultLightActor");
	light_actor->addComponent(std::move(default_light));
	scene.addActor(std::move(light_actor));
}

void AssetImporter::initDefaultCameraControllerComponent(Scene& scene)
{
	auto default_camera = scene.getComponents<CameraComponent>().front();
	auto camera_controller = createDefaultCameraControllerComponent();
	default_camera->getOwner()->addComponent(std::move(camera_controller));
}

AssetHandle<TextureAsset> AssetImporter::getDefaultBaseColorTexture(AssetManager& assets)
{
	static constexpr std::string_view path = "engine://defaults/base_color";
	if (auto existing = assets.findByPath<TextureAsset>(path))
		return existing;

	auto texture = assets.add(createDefaultTextureAsset("Default_Base_Color_Texture", std::string(path)));
	assets.pin(texture);
	return texture;
}

AssetHandle<TextureAsset> AssetImporter::getDefaultMetallicRoughnessTexture(AssetManager& assets)
{
	static constexpr std::string_view path = "engine://defaults/metallic_roughness";
	if (auto existing = assets.findByPath<TextureAsset>(path))
		return existing;

	auto texture = assets.add(createDefaultTextureAsset("Default_Metallic_Roughness_Texture", std::string(path)));
	assets.pin(texture);
	return texture;
}

AssetHandle<MaterialAsset> AssetImporter::getDefaultMaterial(AssetManager& assets)
{
	static constexpr std::string_view path = "engine://defaults/pbr_material";
	if (auto existing = assets.findByPath<MaterialAsset>(path))
		return existing;

	auto base_color = getDefaultBaseColorTexture(assets);
	auto metallic_roughness = getDefaultMetallicRoughnessTexture(assets);
	auto material = createDefaultMaterialAsset("Default_PBR_Material", std::string(path));
	material->setTexture("baseColor", base_color);
	material->setTexture("metallicRoughness", metallic_roughness);
	auto handle = assets.add(std::move(material));
	assets.pin(handle);
	return handle;
}

std::vector<uint8> AssetImporter::getAttributeData(const tinygltf::Model& tfmodel, uint32 accessor_index)
{
	auto view = getAttributeDataView(tfmodel, accessor_index);

	return {view.begin(), view.end()};
}

std::span<const uint8> AssetImporter::getAttributeDataView(const tinygltf::Model& tfmodel, uint32 accessor_index)
{
	assert(accessor_index < tfmodel.accessors.size());
	const auto& accessor = tfmodel.accessors[accessor_index];

	assert(accessor.bufferView < tfmodel.bufferViews.size());
	const auto& buffer_view = tfmodel.bufferViews[accessor.bufferView];

	assert(buffer_view.buffer < tfmodel.buffers.size());
	const auto& buffer = tfmodel.buffers[buffer_view.buffer];

	auto stride = accessor.ByteStride(buffer_view);
	auto start_byte = accessor.byteOffset + buffer_view.byteOffset;
	auto end_byte = start_byte + accessor.count * stride;

	return {buffer.data.begin() + start_byte, buffer.data.begin() + end_byte};
}

uint32 AssetImporter::getAttributeCount(const tinygltf::Model* tfmodel, uint32 accessor_id)
{
	assert(accessor_id < tfmodel->accessors.size());

	return static_cast<uint32>(tfmodel->accessors[accessor_id].count);
}

uint32 AssetImporter::getAttributeSize(const tinygltf::Model* tfmodel, uint32 accessor_id)
{
	assert(accessor_id < tfmodel->accessors.size());
	auto& accessor = tfmodel->accessors[accessor_id];

	size_t component_size = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	size_t component_num = tinygltf::GetNumComponentsInType(accessor.type);

	return static_cast<uint32>(component_size * component_num);
}

uint32 AssetImporter::getAttributeStride(const tinygltf::Model* tfmodel, uint32 accessor_id)
{
	assert(accessor_id < tfmodel->accessors.size());
	auto& accessor = tfmodel->accessors[accessor_id];

	assert(accessor.bufferView < tfmodel->bufferViews.size());
	auto& buffer_view = tfmodel->bufferViews[accessor.bufferView];

	return static_cast<uint32>(accessor.ByteStride(buffer_view));
}

}        // namespace Vortex

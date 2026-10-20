module;

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tiny_gltf.h>

#ifdef ERROR
#	undef ERROR
#endif

module Editor;

namespace Vortex {

[[noreturn]] static void throwAccessorError(std::string_view source_path, int accessor_index, std::string_view message)
{
	ERROR("glTF '{}' accessor {}: {}", source_path.empty() ? "<memory>" : source_path, accessor_index, message);
}

static std::string makeAssetName(std::string_view source_name, std::string_view type, std::string_view virtual_path)
{
	if (!source_name.empty())
		return std::string(source_name);

	const auto separator = virtual_path.find_last_of('/');
	const auto index = separator == std::string_view::npos ? virtual_path : virtual_path.substr(separator + 1);
	return index.empty() ? std::string(type) : std::format("{}_{}", type, index);
}

static const tinygltf::Accessor& requireAccessor(const tinygltf::Model& model, int accessor_index, std::string_view source_path)
{
	if (accessor_index < 0 || static_cast<size_t>(accessor_index) >= model.accessors.size())
		throwAccessorError(source_path, accessor_index, "index is out of range");
	return model.accessors[static_cast<size_t>(accessor_index)];
}

static void requireAccessorFormat(const tinygltf::Model& model, int accessor_index, int component_type,
    int value_type, std::string_view semantic, std::string_view source_path)
{
	const auto& accessor = requireAccessor(model, accessor_index, source_path);
	if (accessor.componentType != component_type || accessor.type != value_type)
		throwAccessorError(source_path, accessor_index, std::format("{} has unsupported component/type layout", semantic));
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
		transform.setMatrix({static_cast<float>(matrix[0]), static_cast<float>(matrix[1]),
		    static_cast<float>(matrix[2]), static_cast<float>(matrix[3]),
		    static_cast<float>(matrix[4]), static_cast<float>(matrix[5]),
		    static_cast<float>(matrix[6]), static_cast<float>(matrix[7]),
		    static_cast<float>(matrix[8]), static_cast<float>(matrix[9]),
		    static_cast<float>(matrix[10]), static_cast<float>(matrix[11]),
		    static_cast<float>(matrix[12]), static_cast<float>(matrix[13]),
		    static_cast<float>(matrix[14]), static_cast<float>(matrix[15])});
}

template <typename S, typename D>
static std::vector<D> convertData(std::span<const uint8> data)
{
	static_assert(sizeof(S) <= sizeof(D),
	    "Source type size must be less than or equal to destination type size");
	CHECK(data.size() % sizeof(S) == 0,
	    "Attribute data is not aligned to its component size");

	const size_t count = data.size() / sizeof(S);
	std::vector<D> result(count);
	for (size_t index = 0; index < count; ++index) {
		S value{};
		std::memcpy(&value, data.data() + index * sizeof(S), sizeof(S));
		result[index] = static_cast<D>(value);
	}
	return result;
}

static std::vector<uint8> getAttributeData(const tinygltf::Model& model, uint32 accessor_index, std::string_view source_path);
static uint32 getAttributeCount(const tinygltf::Model& model, uint32 accessor_index, std::string_view source_path);
static uint32 getAttributeSize(const tinygltf::Model& model, uint32 accessor_index, std::string_view source_path);

static std::unique_ptr<Actor> parseActor(const tinygltf::Node& node);
static std::unique_ptr<MeshComponent> parseMeshComponent(const tinygltf::Mesh& mesh);
static std::unique_ptr<CameraComponent> parseCameraComponent(const tinygltf::Camera& camera);
static std::unique_ptr<LightComponent> parseLightComponent(const tinygltf::Light& light);

static std::shared_ptr<MeshAsset> parseMeshAsset(
    const tinygltf::Mesh& mesh,
    const tinygltf::Model& model,
    const std::vector<AssetHandle<MaterialAsset>>& materials,
    AssetHandle<MaterialAsset> default_material,
    std::string virtual_path);

static std::shared_ptr<TextureAsset> parseTextureAsset(
    const tinygltf::Texture& texture,
    const tinygltf::Model& model,
    std::string virtual_path);

static std::shared_ptr<MaterialAsset> parseMaterialAsset(
    const tinygltf::Material& material,
    const tinygltf::Model& model,
    const std::vector<AssetHandle<TextureAsset>>& textures,
    AssetHandle<TextureAsset> default_base_color,
    AssetHandle<TextureAsset> default_metallic_roughness,
    std::string virtual_path);

static std::unique_ptr<CameraComponent> createDefaultCameraComponent(const std::string& name = "DefaultCameraComponent");
static std::unique_ptr<LightComponent> createDefaultLightComponent(const std::string& name = "DefaultLightComponent");
static std::shared_ptr<TextureAsset> createDefaultTextureAsset(std::string name = "DefaultTexture", std::string virtual_path = {});
static std::shared_ptr<MaterialAsset> createDefaultMaterialAsset(std::string name = "DefaultMaterial", std::string virtual_path = {});
static std::unique_ptr<CameraControllerComponent> createDefaultCameraControllerComponent(
    const std::string& name = "DefaultCameraControllerComponent");

static void initDefaultCameraComponent(World& world);
static void initDefaultLightComponent(World& world);
static void initDefaultCameraControllerComponent(World& world);
static AssetHandle<TextureAsset> getDefaultBaseColorTexture(AssetManager& assets);
static AssetHandle<TextureAsset> getDefaultMetallicRoughnessTexture(AssetManager& assets);
static AssetHandle<MaterialAsset> getDefaultMaterial(AssetManager& assets);

std::unique_ptr<World> loadGltfWorld(std::string_view scene_path)
{
	auto& assets = AssetManager::instance();
	LOG("Loading glTF scene '{}'", scene_path);

	// Load World
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string error, warn;
	const std::string source_path(scene_path);
	if (!loader.LoadASCIIFromFile(&model, &error, &warn, source_path)) {
		if (!error.empty())
			ERROR("Failed to load glTF '{}': {}", source_path, error);
		if (!warn.empty())
			ERROR("Failed to load glTF '{}': {}", source_path, warn);
		ERROR("Failed to load glTF '{}'", source_path);
	}
	if (!warn.empty())
		LOG(Warn, "glTF '{}' loaded with warning: {}", source_path, warn);

	auto world = std::make_unique<World>("Default World");

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

	// Load actors and their owned components.
	std::vector<std::unique_ptr<Actor>> actors;
	for (size_t index = 0; index < model.nodes.size(); index++) {
		const auto& tfnode = model.nodes[index];
		auto actor = parseActor(tfnode);

		if (tfnode.mesh >= 0) {
			CHECK(static_cast<size_t>(tfnode.mesh) < model.meshes.size(),
			    "glTF '{}' node {} references invalid mesh {}",
			    source_path, index, tfnode.mesh);
			auto mesh = parseMeshComponent(model.meshes[tfnode.mesh]);
			mesh->setMesh(meshes[tfnode.mesh]);
			actor->addComponent(std::move(mesh));
		}

		if (tfnode.camera >= 0) {
			CHECK(static_cast<size_t>(tfnode.camera) < model.cameras.size(),
			    "glTF '{}' node {} references invalid camera {}",
			    source_path, index, tfnode.camera);
			actor->addComponent(parseCameraComponent(model.cameras[tfnode.camera]));
		}

		if (tfnode.light >= 0) {
			CHECK(static_cast<size_t>(tfnode.light) < model.lights.size(),
			    "glTF '{}' node {} references invalid light {}",
			    source_path, index, tfnode.light);
			actor->addComponent(parseLightComponent(model.lights[tfnode.light]));
		}

		if (!actor->hasRootComponent())
			actor->addComponent<SceneComponent>("RootSceneComponent");
		applyNodeTransform(actor->getRootComponent()->getTransform(), tfnode);

		actors.push_back(std::move(actor));
	}

	// Build the selected glTF scene hierarchy. Nodes that belong only to other
	// glTF scenes are not registered in this runtime World.
	std::queue<std::pair<Actor&, int>> traverse_actors;

	CHECK(!model.scenes.empty(), "glTF '{}' contains no scene", source_path);
	const size_t scene_index = model.defaultScene >= 0 && model.defaultScene < model.scenes.size() ? static_cast<size_t>(model.defaultScene) : 0;
	const auto& tfscene = model.scenes[scene_index];

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

	std::vector<std::unique_ptr<Actor>> world_actors;
	world_actors.push_back(std::move(root_actor));
	for (size_t index = 0; index < actors.size(); ++index)
		if (active_nodes[index])
			world_actors.push_back(std::move(actors[index]));
	world->setActors(std::move(world_actors));

	initDefaultCameraComponent(*world);
	initDefaultLightComponent(*world);
	initDefaultCameraControllerComponent(*world);

	LOG("Loaded glTF '{}' ({} textures, {} materials, {} meshes, {} actors)",
	    source_path,
	    textures.size(),
	    materials.size(),
	    meshes.size(),
	    world->getActors().size());

	return world;
}

static std::unique_ptr<Actor> parseActor(const tinygltf::Node& tfnode)
{
	return std::make_unique<Actor>(tfnode.name);
}

static std::unique_ptr<MeshComponent> parseMeshComponent(const tinygltf::Mesh& tfmesh)
{
	auto mesh = std::make_unique<MeshComponent>(tfmesh.name);

	return mesh;
}

static std::unique_ptr<CameraComponent> parseCameraComponent(const tinygltf::Camera& tfcamera)
{
	auto camera = std::make_unique<PerspectiveCameraComponent>(tfcamera.name);
	camera->setAspectRatio(static_cast<float>(tfcamera.perspective.aspectRatio));
	camera->setFov(static_cast<float>(tfcamera.perspective.yfov));
	camera->setNearPlane(static_cast<float>(tfcamera.perspective.znear));
	camera->setFarPlane(static_cast<float>(tfcamera.perspective.zfar));

	return camera;
}

static std::unique_ptr<LightComponent> parseLightComponent(const tinygltf::Light& tflight)
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
		ERROR("Unknown glTF light type '{}'", tflight.type);

	light->setColor({static_cast<float>(tflight.color[0]),
	    static_cast<float>(tflight.color[1]),
	    static_cast<float>(tflight.color[2])});
	light->setIntensity(static_cast<float>(tflight.intensity));

	return light;
}

static std::shared_ptr<MeshAsset> parseMeshAsset(const tinygltf::Mesh& tfmesh,
    const tinygltf::Model& tfmodel,
    const std::vector<AssetHandle<MaterialAsset>>& materials,
    AssetHandle<MaterialAsset> default_material,
    std::string virtual_path)
{
	auto mesh = std::make_shared<MeshAsset>(makeAssetName(tfmesh.name, "Mesh", virtual_path), std::move(virtual_path));

	const auto& source_path = mesh->getVirtualPath();

	std::vector<MeshVertex> vertices;
	std::vector<uint32> indices;
	std::vector<MeshSection> sections;
	std::vector<AssetHandle<MaterialAsset>> mesh_materials;

	for (size_t primitive_index = 0; primitive_index < tfmesh.primitives.size(); ++primitive_index) {
		const auto& tfprimitive = tfmesh.primitives[primitive_index];
		CHECK(tfprimitive.mode == -1 || tfprimitive.mode == TINYGLTF_MODE_TRIANGLES,
		    "glTF '{}' mesh primitive {} uses unsupported non-triangle topology",
		    source_path,
		    primitive_index);

		auto position_it = tfprimitive.attributes.find("POSITION");
		CHECK(position_it != tfprimitive.attributes.end(),
		    "glTF '{}' mesh primitive {} has no POSITION accessor",
		    source_path,
		    primitive_index);

		requireAccessorFormat(tfmodel,
		    position_it->second,
		    TINYGLTF_COMPONENT_TYPE_FLOAT,
		    TINYGLTF_TYPE_VEC3,
		    "POSITION",
		    source_path);

		MeshSection section{};
		CHECK(vertices.size() <= std::numeric_limits<uint32>::max(),
		    "glTF '{}' mesh has too many vertices",
		    source_path);
		section.first_vertex = static_cast<uint32>(vertices.size());
		section.vertex_count = getAttributeCount(tfmodel, static_cast<uint32>(position_it->second), source_path);
		if (section.vertex_count == 0 || section.vertex_count > std::numeric_limits<uint32>::max() - section.first_vertex)
			throwAccessorError(source_path, position_it->second, "POSITION count is empty or too large");

		std::vector<MeshVertex> section_vertices(section.vertex_count);
		const auto position_data = getAttributeData(tfmodel, static_cast<uint32>(position_it->second), source_path);
		const auto positions = convertData<float, float>(position_data);
		for (uint32 index = 0; index < section.vertex_count; ++index)
			section_vertices[index].pos = Vec3(positions[index * 3 + 0], positions[index * 3 + 1], positions[index * 3 + 2]);

		if (auto normal_it = tfprimitive.attributes.find("NORMAL"); normal_it != tfprimitive.attributes.end()) {
			requireAccessorFormat(tfmodel,
			    normal_it->second,
			    TINYGLTF_COMPONENT_TYPE_FLOAT,
			    TINYGLTF_TYPE_VEC3,
			    "NORMAL",
			    source_path);
			if (getAttributeCount(tfmodel, static_cast<uint32>(normal_it->second), source_path) != section.vertex_count)
				throwAccessorError(source_path, normal_it->second, "NORMAL count does not match POSITION");
			const auto normal_data = getAttributeData(tfmodel, static_cast<uint32>(normal_it->second), source_path);
			const auto normals = convertData<float, float>(normal_data);
			for (uint32 index = 0; index < section.vertex_count; ++index)
				section_vertices[index].normal = Vec3(normals[index * 3 + 0], normals[index * 3 + 1], normals[index * 3 + 2]);
		}

		if (auto uv_it = tfprimitive.attributes.find("TEXCOORD_0"); uv_it != tfprimitive.attributes.end()) {
			requireAccessorFormat(tfmodel,
			    uv_it->second,
			    TINYGLTF_COMPONENT_TYPE_FLOAT,
			    TINYGLTF_TYPE_VEC2,
			    "TEXCOORD_0",
			    source_path);
			if (getAttributeCount(tfmodel, static_cast<uint32>(uv_it->second), source_path) != section.vertex_count)
				throwAccessorError(source_path, uv_it->second, "TEXCOORD_0 count does not match POSITION");
			const auto uv_data = getAttributeData(tfmodel, static_cast<uint32>(uv_it->second), source_path);
			const auto uvs = convertData<float, float>(uv_data);
			for (uint32 index = 0; index < section.vertex_count; ++index)
				section_vertices[index].uv = Vec2(uvs[index * 2 + 0], uvs[index * 2 + 1]);
		}

		if (auto color_it = tfprimitive.attributes.find("COLOR_0"); color_it != tfprimitive.attributes.end()) {
			const auto& color_accessor = requireAccessor(tfmodel, color_it->second, source_path);
			if (color_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
			    (color_accessor.type != TINYGLTF_TYPE_VEC3 && color_accessor.type != TINYGLTF_TYPE_VEC4))
				throwAccessorError(source_path, color_it->second, "COLOR_0 has unsupported component/type layout");
			if (getAttributeCount(tfmodel, static_cast<uint32>(color_it->second), source_path) != section.vertex_count)
				throwAccessorError(source_path, color_it->second, "COLOR_0 count does not match POSITION");
			const auto color_data = getAttributeData(tfmodel, static_cast<uint32>(color_it->second), source_path);
			const auto colors = convertData<float, float>(color_data);
			const auto component_count = color_accessor.type == TINYGLTF_TYPE_VEC3 ? 3U : 4U;
			for (uint32 index = 0; index < section.vertex_count; ++index) {
				const auto offset = index * component_count;
				section_vertices[index].color = Vec4(
				    colors[offset], colors[offset + 1], colors[offset + 2], component_count == 4 ? colors[offset + 3] : 1.0f);
			}
		}

		vertices.insert(vertices.end(), section_vertices.begin(), section_vertices.end());
		CHECK(indices.size() <= std::numeric_limits<uint32>::max(),
		    "glTF '{}' mesh has too many indices",
		    source_path);
		section.first_index = static_cast<uint32>(indices.size());

		std::vector<uint32> section_indices;
		if (tfprimitive.indices >= 0) {
			const auto& index_accessor = requireAccessor(tfmodel, tfprimitive.indices, source_path);
			if (index_accessor.type != TINYGLTF_TYPE_SCALAR)
				throwAccessorError(source_path, tfprimitive.indices, "index accessor must be SCALAR");
			const auto indices_raw_data = getAttributeData(tfmodel, static_cast<uint32>(tfprimitive.indices), source_path);

			switch (index_accessor.componentType) {
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				section_indices = convertData<uint8, uint32>(indices_raw_data);
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				section_indices = convertData<uint16, uint32>(indices_raw_data);
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				section_indices = convertData<uint32, uint32>(indices_raw_data);
				break;
			default:
				throwAccessorError(source_path, tfprimitive.indices, "index component type is unsupported");
			}
		} else {
			section_indices.resize(section.vertex_count);
			std::iota(section_indices.begin(), section_indices.end(), 0U);
		}
		CHECK(!section_indices.empty() && section_indices.size() % 3 == 0,
		    "glTF '{}' mesh primitive {} does not contain complete triangles",
		    source_path,
		    primitive_index);
		CHECK(!std::ranges::any_of(section_indices, [&section](uint32 index) { return index >= section.vertex_count; }),
		    "glTF '{}' mesh primitive {} contains an out-of-range index",
		    source_path,
		    primitive_index);
		CHECK(section_indices.size() <= std::numeric_limits<uint32>::max() - section.first_index,
		    "glTF '{}' mesh has too many indices",
		    source_path);

		section.index_count = static_cast<uint32>(section_indices.size());
		indices.insert(indices.end(), section_indices.begin(), section_indices.end());

		section.material_slot = static_cast<uint32>(mesh_materials.size());
		if (tfprimitive.material >= 0) {
			CHECK(static_cast<size_t>(tfprimitive.material) < materials.size(),
			    "glTF '{}' mesh primitive {} has an out-of-range material",
			    source_path,
			    primitive_index);
			mesh_materials.push_back(materials[static_cast<size_t>(tfprimitive.material)]);
		} else {
			mesh_materials.push_back(default_material);
		}
		CHECK(mesh_materials.back(),
		    "glTF '{}' mesh primitive {} resolved to an empty material",
		    source_path,
		    primitive_index);
		sections.push_back(section);
	}

	mesh->setVertices(std::move(vertices));
	mesh->setIndices(std::move(indices));
	mesh->setSections(std::move(sections));
	mesh->setMaterials(std::move(mesh_materials));
	CHECK(mesh->valid(), "glTF '{}' did not produce a valid mesh", source_path);
	return mesh;
}

static std::shared_ptr<TextureAsset> parseTextureAsset(const tinygltf::Texture& tftexture,
    const tinygltf::Model& tfmodel,
    std::string virtual_path)
{
	auto texture = std::make_shared<TextureAsset>(
	    makeAssetName(tftexture.name, "Texture", virtual_path), TextureAsset::Dimension::Tex2D, std::move(virtual_path));
	const auto& source_path = texture->getVirtualPath();

	CHECK(tftexture.source >= 0 && tftexture.source < static_cast<int>(tfmodel.images.size()),
	    "glTF '{}' texture source is out of range",
	    source_path);

	const auto& tfimage = tfmodel.images[tftexture.source];
	CHECK(tfimage.width > 0 && tfimage.height > 0 && !tfimage.image.empty(),
	    "glTF '{}' image data is empty or has an invalid extent",
	    source_path);
	CHECK(tfimage.component >= 1 && tfimage.component <= 4,
	    "glTF '{}' image component count is unsupported",
	    source_path);
	CHECK(tfimage.bits <= 0 || tfimage.bits == 8,
	    "glTF '{}' only supports 8-bit texture input",
	    source_path);

	const auto width = static_cast<size_t>(tfimage.width);
	const auto height = static_cast<size_t>(tfimage.height);
	CHECK(height <= std::numeric_limits<size_t>::max() / width,
	    "glTF '{}' image extent overflows storage",
	    source_path);
	const auto pixel_count = width * height;
	CHECK(
	    pixel_count <= std::numeric_limits<size_t>::max() / static_cast<size_t>(tfimage.component) &&
	        tfimage.image.size() == pixel_count * static_cast<size_t>(tfimage.component),
	    "glTF '{}' image byte count does not match its descriptor",
	    source_path);
	CHECK(pixel_count <= std::numeric_limits<size_t>::max() / 4,
	    "glTF '{}' RGBA8 image size overflows storage",
	    source_path);

	texture->setWidth(static_cast<uint32>(tfimage.width));
	texture->setHeight(static_cast<uint32>(tfimage.height));
	texture->setFormat(TexturePixelFormat::RGBA8);

	std::vector<uint8> rgba_data(pixel_count * 4);
	for (size_t pixel = 0; pixel < pixel_count; ++pixel) {
		const auto source = pixel * static_cast<size_t>(tfimage.component);
		const auto destination = pixel * 4;
		switch (tfimage.component) {
		case 1:
			rgba_data[destination + 0] = tfimage.image[source];
			rgba_data[destination + 1] = tfimage.image[source];
			rgba_data[destination + 2] = tfimage.image[source];
			rgba_data[destination + 3] = 255;
			break;
		case 2:
			rgba_data[destination + 0] = tfimage.image[source];
			rgba_data[destination + 1] = tfimage.image[source];
			rgba_data[destination + 2] = tfimage.image[source];
			rgba_data[destination + 3] = tfimage.image[source + 1];
			break;
		case 3:
			rgba_data[destination + 0] = tfimage.image[source];
			rgba_data[destination + 1] = tfimage.image[source + 1];
			rgba_data[destination + 2] = tfimage.image[source + 2];
			rgba_data[destination + 3] = 255;
			break;
		case 4:
			std::memcpy(rgba_data.data() + destination, tfimage.image.data() + source, 4);
			break;
		default:
			std::unreachable();
		}
	}

	texture->setData(std::move(rgba_data));

	return texture;
}

static std::shared_ptr<MaterialAsset> parseMaterialAsset(const tinygltf::Material& tfmaterial,
    const tinygltf::Model& tfmodel,
    const std::vector<AssetHandle<TextureAsset>>& textures,
    AssetHandle<TextureAsset> default_base_color,
    AssetHandle<TextureAsset> default_metallic_roughness,
    std::string virtual_path)
{
	auto material = std::make_shared<MaterialAsset>(
	    makeAssetName(tfmaterial.name, "Material", virtual_path), MaterialAsset::ShadingModel::Lit, std::move(virtual_path));
	const auto& source_path = material->getVirtualPath();
	static_cast<void>(tfmodel);

	auto resolveTexture = [&](int index, std::string_view semantic) -> AssetHandle<TextureAsset> {
		if (index < 0)
			return {};
		CHECK(static_cast<size_t>(index) < textures.size(),
		    "glTF '{}' material {} texture index is out of range",
		    source_path,
		    semantic);
		return textures[static_cast<size_t>(index)];
	};

	const auto& pbr = tfmaterial.pbrMetallicRoughness;
	material->setAlbedo(
	    {static_cast<float>(pbr.baseColorFactor[0]),
	        static_cast<float>(pbr.baseColorFactor[1]),
	        static_cast<float>(pbr.baseColorFactor[2]),
	        static_cast<float>(pbr.baseColorFactor[3])});

	material->setMetallic(static_cast<float>(pbr.metallicFactor));
	material->setRoughness(static_cast<float>(pbr.roughnessFactor));

	if (auto texture = resolveTexture(pbr.baseColorTexture.index, "base color"))
		material->setTexture(MaterialTextureSlot::BaseColor, std::move(texture));
	else
		material->setTexture(MaterialTextureSlot::BaseColor, std::move(default_base_color));

	if (auto texture = resolveTexture(pbr.metallicRoughnessTexture.index, "metallic/roughness"))
		material->setTexture(MaterialTextureSlot::MetallicRoughness, std::move(texture));
	else
		material->setTexture(MaterialTextureSlot::MetallicRoughness, std::move(default_metallic_roughness));

	if (auto texture = resolveTexture(tfmaterial.normalTexture.index, "normal"))
		material->setTexture(MaterialTextureSlot::Normal, std::move(texture));
	if (auto texture = resolveTexture(tfmaterial.occlusionTexture.index, "occlusion"))
		material->setTexture(MaterialTextureSlot::Occlusion, std::move(texture));
	if (auto texture = resolveTexture(tfmaterial.emissiveTexture.index, "emissive"))
		material->setTexture(MaterialTextureSlot::Emissive, std::move(texture));

	material->setEmissive({static_cast<float>(tfmaterial.emissiveFactor[0]),
	    static_cast<float>(tfmaterial.emissiveFactor[1]),
	    static_cast<float>(tfmaterial.emissiveFactor[2])});

	if (tfmaterial.alphaMode == "BLEND")
		material->setAlphaMode(MaterialAsset::AlphaMode::Blend);
	else if (tfmaterial.alphaMode == "OPAQUE")
		material->setAlphaMode(MaterialAsset::AlphaMode::Opaque);
	else if (tfmaterial.alphaMode == "MASK")
		material->setAlphaMode(MaterialAsset::AlphaMode::Mask);
	else
		ERROR("glTF '{}' material alpha mode '{}' is unsupported", source_path, tfmaterial.alphaMode);

	material->setAlphaCutoff(static_cast<float>(tfmaterial.alphaCutoff));
	material->setDoubleSided(tfmaterial.doubleSided);

	return material;
}

static std::unique_ptr<CameraComponent> createDefaultCameraComponent(const std::string& name)
{
	auto camera = std::make_unique<PerspectiveCameraComponent>(name);
	camera->setAspectRatio(16.0f / 9.0f);
	camera->setFov(Math::radians(45.0f));
	camera->setNearPlane(0.1f);
	camera->setFarPlane(1000.0f);

	return camera;
}

static std::unique_ptr<LightComponent> createDefaultLightComponent(const std::string& name)
{
	auto light = std::make_unique<DirectionalLightComponent>(name);
	light->setColor({1.0f, 1.0f, 1.0f});
	light->setIntensity(1.0f);

	return light;
}

static std::shared_ptr<TextureAsset> createDefaultTextureAsset(std::string name, std::string virtual_path)
{
	auto texture = std::make_shared<TextureAsset>(std::move(name), TextureAsset::Dimension::Tex2D, std::move(virtual_path));
	texture->setWidth(1);
	texture->setHeight(1);
	texture->setFormat(TexturePixelFormat::RGBA8);
	texture->setData({255, 255, 255, 255});

	return texture;
}

static std::shared_ptr<MaterialAsset> createDefaultMaterialAsset(std::string name, std::string virtual_path)
{
	auto material = std::make_shared<MaterialAsset>(std::move(name), MaterialAsset::ShadingModel::Lit, std::move(virtual_path));
	material->setAlbedo({1.0f, 1.0f, 1.0f, 1.0f});
	material->setMetallic(0.0f);
	material->setRoughness(1.0f);
	material->setAlphaMode(MaterialAsset::AlphaMode::Opaque);
	material->setDoubleSided(false);

	return material;
}

static std::unique_ptr<CameraControllerComponent> createDefaultCameraControllerComponent(
    const std::string& name)
{
	auto controller = std::make_unique<CameraControllerComponent>(name);

	return controller;
}

static void initDefaultCameraComponent(World& world)
{
	if (world.hasComponent<CameraComponent>())
		return;

	auto default_camera = createDefaultCameraComponent();
	auto camera_actor = std::make_unique<Actor>("DefaultCameraActor");
	camera_actor->addComponent(std::move(default_camera));
	world.addActor(std::move(camera_actor));
}

static void initDefaultLightComponent(World& world)
{
	if (world.hasComponent<LightComponent>())
		return;

	auto default_light = createDefaultLightComponent();
	auto light_actor = std::make_unique<Actor>("DefaultLightActor");
	light_actor->addComponent(std::move(default_light));
	world.addActor(std::move(light_actor));
}

static void initDefaultCameraControllerComponent(World& world)
{
	auto default_camera = world.getComponents<CameraComponent>().front();
	auto camera_controller = createDefaultCameraControllerComponent();
	default_camera->getOwner()->addComponent(std::move(camera_controller));
}

static AssetHandle<TextureAsset> getDefaultBaseColorTexture(AssetManager& assets)
{
	static constexpr std::string_view path = "Engine/Defaults/BaseColor";
	if (auto existing = assets.findByPath<TextureAsset>(path))
		return existing;

	auto texture = assets.add(createDefaultTextureAsset("Default_Base_Color_Texture", std::string(path)));
	assets.pin(texture);
	return texture;
}

static AssetHandle<TextureAsset> getDefaultMetallicRoughnessTexture(AssetManager& assets)
{
	static constexpr std::string_view path = "Engine/Defaults/MetallicRoughness";
	if (auto existing = assets.findByPath<TextureAsset>(path))
		return existing;

	auto texture = assets.add(createDefaultTextureAsset("Default_Metallic_Roughness_Texture", std::string(path)));
	assets.pin(texture);
	return texture;
}

static AssetHandle<MaterialAsset> getDefaultMaterial(AssetManager& assets)
{
	static constexpr std::string_view path = "Engine/Defaults/PBRMaterial";
	if (auto existing = assets.findByPath<MaterialAsset>(path))
		return existing;

	auto base_color = getDefaultBaseColorTexture(assets);
	auto metallic_roughness = getDefaultMetallicRoughnessTexture(assets);
	auto material = createDefaultMaterialAsset("Default_PBR_Material", std::string(path));
	material->setTexture(MaterialTextureSlot::BaseColor, base_color);
	material->setTexture(MaterialTextureSlot::MetallicRoughness, metallic_roughness);
	auto handle = assets.add(std::move(material));
	assets.pin(handle);
	return handle;
}

static std::vector<uint8> getAttributeData(
    const tinygltf::Model& tfmodel, uint32 accessor_index, std::string_view source_path)
{
	const auto& accessor = requireAccessor(tfmodel, static_cast<int>(accessor_index), source_path);
	if (accessor.sparse.isSparse)
		throwAccessorError(source_path, static_cast<int>(accessor_index), "sparse accessors are unsupported");
	if (accessor.normalized)
		throwAccessorError(source_path, static_cast<int>(accessor_index), "normalized accessors are unsupported");
	if (accessor.bufferView < 0 || static_cast<size_t>(accessor.bufferView) >= tfmodel.bufferViews.size())
		throwAccessorError(source_path, static_cast<int>(accessor_index), "buffer view is missing or out of range");

	const auto& buffer_view = tfmodel.bufferViews[static_cast<size_t>(accessor.bufferView)];
	if (buffer_view.buffer < 0 || static_cast<size_t>(buffer_view.buffer) >= tfmodel.buffers.size())
		throwAccessorError(source_path, static_cast<int>(accessor_index), "buffer is out of range");
	const auto& buffer = tfmodel.buffers[static_cast<size_t>(buffer_view.buffer)];

	const auto element_size = static_cast<size_t>(getAttributeSize(tfmodel, accessor_index, source_path));
	const int raw_stride = accessor.ByteStride(buffer_view);
	if (raw_stride <= 0 || static_cast<size_t>(raw_stride) < element_size)
		throwAccessorError(source_path, static_cast<int>(accessor_index), "byteStride is invalid for the element layout");
	const auto stride = static_cast<size_t>(raw_stride);

	if (buffer_view.byteOffset > buffer.data.size() ||
	    buffer_view.byteLength > buffer.data.size() - buffer_view.byteOffset)
		throwAccessorError(source_path, static_cast<int>(accessor_index), "buffer view exceeds its buffer");
	if (accessor.byteOffset > buffer_view.byteLength)
		throwAccessorError(source_path, static_cast<int>(accessor_index), "byteOffset exceeds its buffer view");

	if (accessor.count > 0) {
		if (accessor.count - 1 > (std::numeric_limits<size_t>::max() - element_size) / stride)
			throwAccessorError(source_path, static_cast<int>(accessor_index), "byte range overflows storage");
		const auto required_size = (accessor.count - 1) * stride + element_size;
		if (required_size > buffer_view.byteLength - accessor.byteOffset)
			throwAccessorError(source_path, static_cast<int>(accessor_index), "data exceeds its buffer view");
	}
	if (accessor.count > std::numeric_limits<size_t>::max() / element_size)
		throwAccessorError(source_path, static_cast<int>(accessor_index), "packed data size overflows storage");

	std::vector<uint8> result(accessor.count * element_size);
	const auto start = buffer_view.byteOffset + accessor.byteOffset;
	for (size_t element = 0; element < accessor.count; ++element)
		std::memcpy(result.data() + element * element_size,
		    buffer.data.data() + start + element * stride,
		    element_size);
	return result;
}

static uint32 getAttributeCount(
    const tinygltf::Model& tfmodel, uint32 accessor_id, std::string_view source_path)
{
	const auto& accessor = requireAccessor(tfmodel, static_cast<int>(accessor_id), source_path);
	if (accessor.count > std::numeric_limits<uint32>::max())
		throwAccessorError(source_path, static_cast<int>(accessor_id), "element count exceeds runtime limits");
	return static_cast<uint32>(accessor.count);
}

static uint32 getAttributeSize(
    const tinygltf::Model& tfmodel, uint32 accessor_id, std::string_view source_path)
{
	const auto& accessor = requireAccessor(tfmodel, static_cast<int>(accessor_id), source_path);
	const auto component_size = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	const auto component_count = tinygltf::GetNumComponentsInType(accessor.type);
	if (component_size <= 0 || component_count <= 0 ||
	    component_size > std::numeric_limits<uint32>::max() / component_count)
		throwAccessorError(source_path, static_cast<int>(accessor_id), "component/type layout is invalid");
	return static_cast<uint32>(component_size * component_count);
}

}        // namespace Vortex

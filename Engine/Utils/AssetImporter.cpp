#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "AssetImporter.hpp"

#include <queue>
#include <stdexcept>

static constexpr std::array attributes_names = {
    "POSITION",
    "NORMAL",
    "TEXCOORD_0",
    "COLOR_0",
};

std::weak_ptr<Material> AssetImporter::default_pbr_material{};
std::weak_ptr<Texture>  AssetImporter::default_base_color_texture{};
std::weak_ptr<Texture>  AssetImporter::default_metallic_roughness_texture{};

std::unique_ptr<Scene> AssetImporter::loadScene(std::string_view scene_path)
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

	// Load Cameras
	std::vector<std::unique_ptr<Camera>> cameras;
	for (const auto& tfcamera : model.cameras)
		cameras.push_back(parseCamera(tfcamera));
	if (!cameras.empty())
		scene->setComponents(std::move(cameras));

	// Load Lights
	std::vector<std::unique_ptr<Light>> lights;
	for (const auto& tflight : model.lights)
		lights.push_back(parseLight(tflight));
	if (!lights.empty())
		scene->setComponents(std::move(lights));

	// Load Textures
	std::vector<std::shared_ptr<Texture>> textures;
	for (const auto& tftexture : model.textures)
		textures.push_back(parseTexture(tftexture, model));
	if (!textures.empty())
		scene->setResources(std::move(textures));
	initDefaultTextures(*scene);

	// Load Materials
	std::vector<std::shared_ptr<Material>> materials;
	for (size_t i = 0; i < model.materials.size(); i++)
		materials.push_back(parseMaterial(model.materials[i], model, scene->getResources<Texture>()));
	if (!materials.empty())
		scene->setResources(std::move(materials));
	initDefaultMaterials(*scene);

	// Load Meshes
	for (const auto& tfmesh : model.meshes) {
		auto mesh = parseMesh(tfmesh);
		for (auto index = 0; index < tfmesh.primitives.size(); index++) {
			auto submesh = parseSubmesh(tfmesh, model, index, scene->getResources<Material>());
			auto shared_submesh = std::shared_ptr<SubMesh>(std::move(submesh));
			mesh->addSubmesh(shared_submesh);
			scene->addResource<SubMesh>(shared_submesh);
		}
		scene->addComponent(std::move(mesh));
	}

	// Load Nodes
	std::vector<std::unique_ptr<Node>> nodes;
	for (size_t index = 0; index < model.nodes.size(); index++) {
		auto tfnode = model.nodes[index];
		auto node = parseNode(tfnode);

		if (tfnode.mesh >= 0) {
			auto meshes = scene->getComponents<Mesh>();
			assert(tfnode.mesh < meshes.size());
			auto mesh = meshes[tfnode.mesh];
			node->setComponent(*mesh);
			mesh->setNode(*node);
		}

		if (tfnode.camera >= 0) {
			auto cameras = scene->getComponents<Camera>();
			assert(tfnode.camera < cameras.size());
			auto camera = cameras[tfnode.camera];
			node->setComponent(*camera);
			camera->setNode(*node);
		}

		if (tfnode.light >= 0) {
			auto lights = scene->getComponents<Light>();
			assert(tfnode.light < lights.size());
			auto light = lights[tfnode.light];
			node->setComponent(*light);
			light->setNode(*node);
		}

		nodes.push_back(std::move(node));
	}

	// Load Scenes
	std::queue<std::pair<Node&, int>> traverse_nodes;

	tinygltf::Scene* tfscene = &model.scenes.front();
	if (!tfscene)
		throw std::runtime_error("No default scene found in glTF file");

	auto root_node = std::make_unique<Node>(tfscene->name);
	for (auto node_index : tfscene->nodes)
		traverse_nodes.push({std::ref(*root_node), node_index});

	while (!traverse_nodes.empty()) {
		auto node_it = traverse_nodes.front();
		traverse_nodes.pop();
		if (node_it.second >= nodes.size())
			continue;

		auto& current_node = *nodes.at(node_it.second);
		auto& traverse_root_node = node_it.first;
		traverse_root_node.addChild(current_node);

		for (auto child_node_index : model.nodes.at(node_it.second).children)
			traverse_nodes.push({std::ref(current_node), child_node_index});
	}

	scene->setRoot(*root_node);
	nodes.insert(nodes.begin(), std::move(root_node));
	scene->setNodes(std::move(nodes));

	initDefaultCamera(*scene);
	initDefaultLight(*scene);
	initDefaultCameraController(*scene);

	return scene;
}

std::unique_ptr<Node> AssetImporter::parseNode(const tinygltf::Node& tfnode)
{
	auto  node = std::make_unique<Node>(tfnode.name);
	auto& transform = node->getTransform();

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

	if (const auto& matrix = tfnode.matrix; !tfnode.matrix.empty())
		transform.setMatrix({static_cast<float>(matrix[0]), static_cast<float>(matrix[1]), static_cast<float>(matrix[2]), static_cast<float>(matrix[3]),
		    static_cast<float>(matrix[4]), static_cast<float>(matrix[5]), static_cast<float>(matrix[6]), static_cast<float>(matrix[7]),
		    static_cast<float>(matrix[8]), static_cast<float>(matrix[9]), static_cast<float>(matrix[10]), static_cast<float>(matrix[11]),
		    static_cast<float>(matrix[12]), static_cast<float>(matrix[13]), static_cast<float>(matrix[14]), static_cast<float>(matrix[15])});

	return node;
}

std::unique_ptr<Mesh> AssetImporter::parseMesh(const tinygltf::Mesh& tfmesh)
{
	auto mesh = std::make_unique<Mesh>(tfmesh.name);

	return mesh;
}

std::unique_ptr<Camera> AssetImporter::parseCamera(const tinygltf::Camera& tfcamera)
{
	auto camera = std::make_unique<PerspectiveCamera>(tfcamera.name);
	camera->setAspectRatio(static_cast<float>(tfcamera.perspective.aspectRatio));
	camera->setFov(static_cast<float>(tfcamera.perspective.yfov));
	camera->setNearPlane(static_cast<float>(tfcamera.perspective.znear));
	camera->setFarPlane(static_cast<float>(tfcamera.perspective.zfar));

	return camera;
}

std::unique_ptr<Light> AssetImporter::parseLight(const tinygltf::Light& tflight)
{
	std::unique_ptr<Light> light{};
	if (tflight.type == "directional") {
		light = std::make_unique<DirectionalLight>(tflight.name);

	} else if (tflight.type == "point") {
		light = std::make_unique<PointLight>(tflight.name);
		dynamic_cast<PointLight*>(light.get())->setRange(static_cast<float>(tflight.range));

	} else if (tflight.type == "spot") {
		light = std::make_unique<SpotLight>(tflight.name);
		dynamic_cast<SpotLight*>(light.get())->setRange(static_cast<float>(tflight.range));
		dynamic_cast<SpotLight*>(light.get())->setInnerConeAngle(static_cast<float>(tflight.spot.innerConeAngle));
		dynamic_cast<SpotLight*>(light.get())->setOuterConeAngle(static_cast<float>(tflight.spot.outerConeAngle));

	} else
		throw std::runtime_error("Unknown light type");

	light->setColor({static_cast<float>(tflight.color[0]),
	    static_cast<float>(tflight.color[1]),
	    static_cast<float>(tflight.color[2])});
	light->setIntensity(static_cast<float>(tflight.intensity));

	return light;
}

std::shared_ptr<SubMesh> AssetImporter::parseSubmesh(const tinygltf::Mesh& tfmesh, const tinygltf::Model& tfmodel, uint32_t index, const std::vector<std::shared_ptr<Material>>& materials)
{
	auto submesh = std::make_shared<SubMesh>(tfmesh.name);

	const auto& tfprimitive = tfmesh.primitives[index];

	// Load Vertices
	struct AttributeData {
		std::string              name;
		uint32_t                 size;
		std::span<const uint8_t> data_view;
	};

	std::map<std::string, AttributeData> attributes_map;
	for (const auto& [attr_name, accessor_id] : tfprimitive.attributes) {
		auto upper_name = attr_name;
		std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);
		if (std::find(attributes_names.begin(), attributes_names.end(), upper_name) == attributes_names.end())
			continue;

		attributes_map[upper_name] = AttributeData{
		    .name = upper_name,
		    .size = getAttributeSize(&tfmodel, accessor_id),
		    .data_view = getAttributeDataView(tfmodel, accessor_id),
		};
	}

	uint32_t vertex_count = getAttributeCount(&tfmodel, tfprimitive.attributes.begin()->second);
	uint32_t vertex_stride = 0;
	for (const auto& attribute_name : attributes_names) {
		if (!attributes_map.contains(attribute_name))
			continue;

		auto&    attribute = attributes_map.at(attribute_name);
		uint32_t offset = vertex_stride;

		submesh->setAttribute(attribute_name, {attribute.size, offset});
		vertex_stride += attribute.size;
	}

	std::vector<uint8_t> vertices_raw_data(vertex_count * vertex_stride);
	for (const auto& attribute_name : attributes_names) {
		if (!attributes_map.contains(attribute_name))
			continue;

		const auto& attribute = attributes_map.at(attribute_name);
		const auto& submesh_attribute = submesh->getAttribute(attribute_name);
		uint32_t    offset = submesh_attribute->offset;

		for (uint32_t vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
			std::memcpy(&vertices_raw_data[vertex_index * vertex_stride + offset],
			    &attribute.data_view[vertex_index * attribute.size],
			    attribute.size);
		}
	}

	std::vector<float> vertices_data(vertices_raw_data.size() / sizeof(float));
	std::memcpy(vertices_data.data(), vertices_raw_data.data(), vertices_raw_data.size());
	submesh->setVertices(std::move(vertices_data), vertex_count);

	// Load Indices
	if (tfprimitive.indices >= 0) {
		auto indices_raw_data = getAttributeDataView(tfmodel, tfprimitive.indices);
		auto index_byte_size = getAttributeSize(&tfmodel, tfprimitive.indices);

		std::vector<uint32_t> indices_data;
		switch (index_byte_size) {
		case 1:
			indices_data = convertData<uint8_t, uint32_t>(indices_raw_data);
			break;
		case 2:
			indices_data = convertData<uint16_t, uint32_t>(indices_raw_data);
			break;
		case 4:
			indices_data = convertData<uint32_t, uint32_t>(indices_raw_data);
			break;
		default:
			throw std::runtime_error("Unsupported index byte size");
		}

		submesh->setIndices(std::move(indices_data));
	}

	// Load Materials
	if (tfprimitive.material >= 0 && tfprimitive.material < tfmodel.materials.size())
		submesh->setMaterial(materials[tfprimitive.material]);
	else
		submesh->setMaterial(default_pbr_material.lock());

	return submesh;
}

std::shared_ptr<Texture> AssetImporter::parseTexture(const tinygltf::Texture& tftexture, const tinygltf::Model& tfmodel)
{
	auto texture = std::make_shared<Texture>(tftexture.name);

	const auto& tfimage = tfmodel.images[tftexture.source];
	if (tftexture.source < 0 || tftexture.source >= static_cast<int>(tfmodel.images.size()))
		return texture;
	else if (tfimage.width == 0 || tfimage.height == 0 || tfimage.image.empty())
		return texture;
	else if (tfimage.component != 3 && tfimage.component != 4)
		throw std::runtime_error("Unsupported image component count");

	texture->setWidth(tfimage.width);
	texture->setHeight(tfimage.height);
	texture->setFormat(4);

	std::vector<uint8_t> rgba_data;
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

std::shared_ptr<Material> AssetImporter::parseMaterial(const tinygltf::Material& tfmaterial, const tinygltf::Model& tfmodel, const std::vector<std::shared_ptr<Texture>>& textures)
{
	auto material = std::make_shared<PBRMaterial>(tfmaterial.name);

	const auto& pbr = tfmaterial.pbrMetallicRoughness;
	material->setBaseColorFactor(
	    {static_cast<float>(pbr.baseColorFactor[0]),
	        static_cast<float>(pbr.baseColorFactor[1]),
	        static_cast<float>(pbr.baseColorFactor[2]),
	        static_cast<float>(pbr.baseColorFactor[3])});

	material->setMetallicFactor(static_cast<float>(pbr.metallicFactor));
	material->setRoughnessFactor(static_cast<float>(pbr.roughnessFactor));

	if (pbr.baseColorTexture.index >= 0 && pbr.baseColorTexture.index < textures.size())
		material->addTexture("baseColor", textures[pbr.baseColorTexture.index]);
	else
		material->addTexture("baseColor", default_base_color_texture.lock());

	if (pbr.metallicRoughnessTexture.index >= 0 && pbr.metallicRoughnessTexture.index < textures.size())
		material->addTexture("metallicRoughness", textures[pbr.metallicRoughnessTexture.index]);
	else
		material->addTexture("metallicRoughness", default_metallic_roughness_texture.lock());

	material->setEmissive({static_cast<float>(tfmaterial.emissiveFactor[0]),
	    static_cast<float>(tfmaterial.emissiveFactor[1]),
	    static_cast<float>(tfmaterial.emissiveFactor[2])});

	if (tfmaterial.alphaMode == "BLEND")
		material->setAlphaMode(AlphaMode::Blend);
	else if (tfmaterial.alphaMode == "OPAQUE")
		material->setAlphaMode(AlphaMode::Opaque);
	else if (tfmaterial.alphaMode == "MASK")
		material->setAlphaMode(AlphaMode::Mask);

	material->setAlphaCutoff(static_cast<float>(tfmaterial.alphaCutoff));
	material->setDoubleSided(tfmaterial.doubleSided);

	return material;
}

std::unique_ptr<Camera> AssetImporter::createDefaultCamera(const std::string& name)
{
	auto camera = std::make_unique<PerspectiveCamera>(name);
	camera->setAspectRatio(16.0f / 9.0f);
	camera->setFov(glm::radians(45.0f));
	camera->setNearPlane(0.1f);
	camera->setFarPlane(1000.0f);

	return camera;
}

std::unique_ptr<Light> AssetImporter::createDefaultLight(const std::string& name)
{
	auto light = std::make_unique<DirectionalLight>(name);
	light->setColor({1.0f, 1.0f, 1.0f});
	light->setIntensity(1.0f);

	return light;
}

std::shared_ptr<Texture> AssetImporter::createDefaultTexture(const std::string& name)
{
	auto texture = std::make_shared<Texture>(name);
	texture->setWidth(1);
	texture->setHeight(1);
	texture->setFormat(4);
	texture->setData({255, 255, 255, 255});

	return texture;
}

std::shared_ptr<Material> AssetImporter::createDefaultMaterial(const std::string& name)
{
	auto material = std::make_shared<PBRMaterial>(name);
	material->setBaseColorFactor({1.0f, 1.0f, 1.0f, 1.0f});
	material->setMetallicFactor(0.0f);
	material->setRoughnessFactor(1.0f);
	material->setAlphaMode(AlphaMode::Opaque);
	material->setDoubleSided(false);

	return material;
}

std::unique_ptr<CameraController> AssetImporter::createDefaultCameraController(const std::string& name)
{
	auto controller = std::make_unique<CameraController>(name);

	return controller;
}

void AssetImporter::initDefaultCamera(Scene& scene)
{
	if (scene.hasComponent<Camera>())
		return;

	auto default_camera = createDefaultCamera("Default_Camera");
	auto camera_node = std::make_unique<Node>("Default_Camera_Node");
	default_camera->setNode(*camera_node);
	camera_node->setComponent(*default_camera);
	scene.addComponent(std::move(default_camera));
	scene.getRoot()->addChild(*camera_node);
	scene.addNode(std::move(camera_node));
}

void AssetImporter::initDefaultLight(Scene& scene)
{
	if (scene.hasComponent<Light>())
		return;

	auto default_light = createDefaultLight("Default_Light");
	auto light_node = std::make_unique<Node>("Default_Light_Node");
	default_light->setNode(*light_node);
	light_node->setComponent(*default_light);
	scene.addComponent(std::move(default_light));
	scene.getRoot()->addChild(*light_node);
	scene.addNode(std::move(light_node));
}

void AssetImporter::initDefaultCameraController(Scene& scene)
{
	auto default_camera = scene.getComponents<Camera>().front();
	auto camera_controller = createDefaultCameraController("Default_Camera_Controller");
	scene.addBehaviour(std::move(camera_controller), *default_camera->getNode());
}

void AssetImporter::initDefaultTextures(Scene& scene)
{
	auto dbct = createDefaultTexture("Default_Base_Color_Texture");
	dbct->setData({255, 255, 255, 255});
	default_base_color_texture = dbct;
	scene.addResource<Texture>(dbct);

	auto dmrt = createDefaultTexture("Default_Metallic_Roughness_Texture");
	dmrt->setData({255, 255, 255, 255});
	default_metallic_roughness_texture = dmrt;
	scene.addResource<Texture>(dmrt);
}

void AssetImporter::initDefaultMaterials(Scene& scene)
{
	auto dpm = createDefaultMaterial("Default_PBR_Material");
	dpm->addTexture("baseColor", default_base_color_texture.lock());
	dpm->addTexture("metallicRoughness", default_metallic_roughness_texture.lock());
	default_pbr_material = dpm;
	scene.addResource<Material>(dpm);
}

std::vector<uint8_t> AssetImporter::getAttributeData(const tinygltf::Model& tfmodel, uint32_t accessor_index)
{
	auto view = getAttributeDataView(tfmodel, accessor_index);

	return {view.begin(), view.end()};
}

std::span<const uint8_t> AssetImporter::getAttributeDataView(const tinygltf::Model& tfmodel, uint32_t accessor_index)
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

uint32_t AssetImporter::getAttributeCount(const tinygltf::Model* tfmodel, uint32_t accessor_id)
{
	assert(accessor_id < tfmodel->accessors.size());

	return static_cast<uint32_t>(tfmodel->accessors[accessor_id].count);
}

uint32_t AssetImporter::getAttributeSize(const tinygltf::Model* tfmodel, uint32_t accessor_id)
{
	assert(accessor_id < tfmodel->accessors.size());
	auto& accessor = tfmodel->accessors[accessor_id];

	size_t component_size = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	size_t component_num = tinygltf::GetNumComponentsInType(accessor.type);

	return static_cast<uint32_t>(component_size * component_num);
}

uint32_t AssetImporter::getAttributeStride(const tinygltf::Model* tfmodel, uint32_t accessor_id)
{
	assert(accessor_id < tfmodel->accessors.size());
	auto& accessor = tfmodel->accessors[accessor_id];

	assert(accessor.bufferView < tfmodel->bufferViews.size());
	auto& buffer_view = tfmodel->bufferViews[accessor.bufferView];

	return static_cast<uint32_t>(accessor.ByteStride(buffer_view));
}

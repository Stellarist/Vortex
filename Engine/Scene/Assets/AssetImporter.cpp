#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "AssetImporter.hpp"

#include <queue>
#include <print>
#include <stdexcept>

static constexpr std::array attributes_names = {
    "POSITION",
    "NORMAL",
    "TEXCOORD_0",
    "COLOR_0",
};

std::unique_ptr<Scene> AssetImporter::loadScene(std::string_view file_path)
{
	// Load Scene
	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        error, warn;
	if (!loader.LoadASCIIFromFile(&model, &error, &warn, file_path.data())) {
		if (!error.empty())
			std::println("Error: {}", error);
		if (!warn.empty())
			std::println("Warning: {}", warn);
		throw std::runtime_error("Failed to load glTF file.");
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

	// Load Materials
	std::vector<std::shared_ptr<Material>> materials;
	for (const auto& tfmaterial : model.materials)
		materials.push_back(parseMaterial(tfmaterial));
	if (!materials.empty())
		scene->setResources(std::move(materials));

	// Load Meshes
	// TODO: Add material loading and assignment
	for (const auto& tfmesh : model.meshes) {
		auto mesh = parseMesh(tfmesh);
		for (auto index = 0; index < tfmesh.primitives.size(); index++) {
			auto submesh = parseSubmesh(tfmesh, model, index);
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
		auto node = parseNode(tfnode, index);

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
		throw std::runtime_error("No default scene found in glTF file.");

	auto root_node = std::make_unique<Node>(-1, tfscene->name);
	for (auto node_index : tfscene->nodes)
		traverse_nodes.push({std::ref(*root_node), node_index});

	while (!traverse_nodes.empty()) {
		auto node_it = traverse_nodes.front();
		traverse_nodes.pop();
		if (node_it.second >= nodes.size())
			continue;

		auto& current_node = *nodes.at(node_it.second);
		auto& traverse_root_node = node_it.first;
		current_node.setParent(traverse_root_node);
		traverse_root_node.addChild(current_node);

		for (auto child_node_index : model.nodes.at(node_it.second).children)
			traverse_nodes.push({std::ref(current_node), child_node_index});
	}

	scene->setRoot(*root_node);
	nodes.insert(nodes.begin(), std::move(root_node));
	scene->setNodes(std::move(nodes));

	// Add Default Camera
	if (!scene->hasComponent<Camera>()) {
		auto camera_node = std::make_unique<Node>(-1, "Default_Camera_Node");
		auto default_camera = createDefaultCamera();
		scene->addComponent<Camera>(std::move(default_camera));
		scene->getRoot().addChild(*camera_node);
		scene->addNode(std::move(camera_node));
	}

	// Add Default Light
	if (!scene->hasComponent<Light>()) {
		auto light_node = std::make_unique<Node>(-1, "Default_Light_Node");
		auto default_light = createDefaultLight();
		scene->addComponent<Light>(std::move(default_light));
		scene->getRoot().addChild(*light_node);
		scene->addNode(std::move(light_node));
	}

	// Add Default Material
	if (!scene->hasResource<Material>()) {
		auto default_material = createDefaultMaterial();
		scene->addResource(std::shared_ptr<Material>(std::move(default_material)));
	}

	for (auto  materials = scene->getResources<Material>();
	     auto& submesh : scene->getResources<SubMesh>()) {
		if (!submesh->getMaterial())
			submesh->setMaterial(materials.front());
	}

	return scene;
}

std::unique_ptr<SubMesh> AssetImporter::loadModel(const tinygltf::Model& tfmodel, uint32_t index)
{
	auto submesh = std::make_unique<SubMesh>();

	auto& tfmesh = tfmodel.meshes[index];
	auto& tfprimitive = tfmesh.primitives[index];

	const float* pos = nullptr;
	const float* normals = nullptr;
	const float* uvs = nullptr;
	const float* colors = nullptr;

	auto& accessor = tfmodel.accessors[tfprimitive.attributes.find("POSITION")->second];
	auto& buffer_view = tfmodel.bufferViews[accessor.bufferView];

	size_t vertex_count = accessor.count;
	pos = reinterpret_cast<const float*>(&tfmodel.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]);

	// submesh->setVerticesCount(static_cast<uint32_t>(vertex_count));

	if (tfprimitive.attributes.find("NORMAL") != tfprimitive.attributes.end()) {
		auto& accessor = tfmodel.accessors[tfprimitive.attributes.find("NORMAL")->second];
		auto& buffer_view = tfmodel.bufferViews[accessor.bufferView];
		normals = reinterpret_cast<const float*>(&tfmodel.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]);
	}

	if (tfprimitive.attributes.find("TEXCOORD_0") != tfprimitive.attributes.end()) {
		auto& accessor = tfmodel.accessors[tfprimitive.attributes.find("TEXCOORD_0")->second];
		auto& buffer_view = tfmodel.bufferViews[accessor.bufferView];
		uvs = reinterpret_cast<const float*>(&tfmodel.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]);
	}

	if (tfprimitive.attributes.find("COLOR_0") != tfprimitive.attributes.end()) {
		auto& accessor = tfmodel.accessors[tfprimitive.attributes.find("COLOR_0")->second];
		auto& buffer_view = tfmodel.bufferViews[accessor.bufferView];
		colors = reinterpret_cast<const float*>(&tfmodel.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]);
	}

	// if (tfprimitive.indices >= 0) {
	// 	auto& accessor = tfmodel.accessors[tfprimitive.indices];
	// 	auto& buffer_view = tfmodel.bufferViews[accessor.bufferView];
	// 	auto* indices = reinterpret_cast<const uint32_t*>(&tfmodel.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]);

	// 	std::vector<uint32_t> index_data(indices, indices + accessor.count);
	// 	submesh->setIndices(std::move(index_data));
	// }

	// TODO: Load mesh data into submesh

	return submesh;
}

std::unique_ptr<Node> AssetImporter::parseNode(const tinygltf::Node& tfnode, size_t index)
{
	auto  node = std::make_unique<Node>(index, tfnode.name);
	auto& transform = node->getTransform();

	if (const auto& translation = tfnode.translation; !translation.empty())
		transform.setTranslation(glm::vec3(translation[0], translation[1], translation[2]));

	if (const auto& rotation = tfnode.rotation; !rotation.empty())
		transform.setRotation(glm::quat(rotation[3], rotation[0], rotation[1], rotation[2]));

	if (const auto& scale = tfnode.scale; !scale.empty())
		transform.setScale(glm::vec3(scale[0], scale[1], scale[2]));

	if (const auto& matrix = tfnode.matrix; !tfnode.matrix.empty())
		transform.setMatrix(glm::mat4(
		    matrix[0], matrix[1], matrix[2], matrix[3],
		    matrix[4], matrix[5], matrix[6], matrix[7],
		    matrix[8], matrix[9], matrix[10], matrix[11],
		    matrix[12], matrix[13], matrix[14], matrix[15]));

	return node;
}

std::unique_ptr<Mesh> AssetImporter::parseMesh(const tinygltf::Mesh& tfmesh)
{
	auto mesh = std::make_unique<Mesh>(tfmesh.name);

	return mesh;
}

std::unique_ptr<SubMesh> AssetImporter::parseSubmesh(const tinygltf::Mesh& tfmesh, const tinygltf::Model& model, uint32_t index)
{
	auto submesh = std::make_unique<SubMesh>(std::format("{}_Primitive_{}", tfmesh.name, index));

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
		    .size = getAttributeSize(&model, accessor_id),
		    .data_view = getAttributeDataView(model, accessor_id),
		};
	}

	uint32_t vertex_count = getAttributeCount(&model, tfprimitive.attributes.begin()->second);
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
		auto indices_raw_data = getAttributeDataView(model, tfprimitive.indices);
		auto index_byte_size = getAttributeSize(&model, tfprimitive.indices);

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

	return submesh;
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
		const auto& range = tflight.range;
		dynamic_cast<PointLight*>(light.get())->setRange(range);
	} else if (tflight.type == "spot") {
		light = std::make_unique<SpotLight>(tflight.name);
		const auto& range = tflight.range;
		const auto& spot = tflight.spot;
		dynamic_cast<SpotLight*>(light.get())->setRange(range);
		dynamic_cast<SpotLight*>(light.get())->setInnerConeAngle(spot.innerConeAngle);
		dynamic_cast<SpotLight*>(light.get())->setOuterConeAngle(spot.outerConeAngle);
	} else
		throw std::runtime_error("Unknown light type");

	const auto& color = tflight.color;
	light->setColor({color[0], color[1], color[2]});

	const auto& intensity = tflight.intensity;
	light->setIntensity(intensity);

	return light;
}

std::unique_ptr<Material> AssetImporter::parseMaterial(const tinygltf::Material& tfmaterial)
{
	auto material = std::make_unique<PBRMaterial>(tfmaterial.name);

	for (auto& tfvalue : tfmaterial.values) {
		if (tfvalue.first == "baseColorFactor") {
			const auto& color = tfvalue.second.ColorFactor();
			material->setBaseColorFactor({color[0], color[1], color[2], color[3]});
		} else if (tfvalue.first == "metallicFactor")
			material->setMetallicFactor(static_cast<float>(tfvalue.second.Factor()));
		else if (tfvalue.first == "roughnessFactor")
			material->setRoughnessFactor(static_cast<float>(tfvalue.second.Factor()));
	}

	for (auto& tfvalue : tfmaterial.additionalValues) {
		if (tfvalue.first == "emissiveFactor") {
			const auto& emissive = tfvalue.second.number_array;
			material->setEmissive({emissive[0], emissive[1], emissive[2]});
		} else if (tfvalue.first == "alphaMode") {
			if (tfvalue.second.string_value == "BLEND")
				material->setAlphaMode(AlphaMode::Blend);
			else if (tfvalue.second.string_value == "OPAQUE")
				material->setAlphaMode(AlphaMode::Opaque);
			else if (tfvalue.second.string_value == "MASK")
				material->setAlphaMode(AlphaMode::Mask);
		} else if (tfvalue.first == "alphaCutoff")
			material->setAlphaCutoff(static_cast<float>(tfvalue.second.number_value));
		else if (tfvalue.first == "doubleSided")
			material->setDoubleSided(tfvalue.second.bool_value);
	}

	return material;
}

std::unique_ptr<Texture> AssetImporter::parseTexture(const tinygltf::Texture& tftexture)
{
	return std::make_unique<Texture>(tftexture.name);
}

std::unique_ptr<Camera> AssetImporter::createDefaultCamera()
{
	auto camera = std::make_unique<PerspectiveCamera>("Default_Camera");
	camera->setAspectRatio(16.0f / 9.0f);
	camera->setFov(glm::radians(45.0f));
	camera->setNearPlane(0.1f);
	camera->setFarPlane(1000.0f);

	return camera;
}

std::unique_ptr<Light> AssetImporter::createDefaultLight()
{
	auto light = std::make_unique<DirectionalLight>("Default_Directional_Light");
	light->setColor({1.0f, 1.0f, 1.0f});
	light->setIntensity(1.0f);

	return light;
}

std::unique_ptr<Material> AssetImporter::createDefaultMaterial()
{
	auto material = std::make_unique<PBRMaterial>("Default_Material");
	material->setBaseColorFactor({1.0f, 1.0f, 1.0f, 1.0f});
	material->setMetallicFactor(0.0f);
	material->setRoughnessFactor(1.0f);
	material->setAlphaMode(AlphaMode::Opaque);
	material->setDoubleSided(false);

	return material;
}

std::vector<uint8_t> AssetImporter::getAttributeData(const tinygltf::Model& model, uint32_t accessor_index)
{
	auto view = getAttributeDataView(model, accessor_index);

	return {view.begin(), view.end()};
}

std::span<const uint8_t> AssetImporter::getAttributeDataView(const tinygltf::Model& model, uint32_t accessor_index)
{
	assert(accessor_index < model.accessors.size());
	const auto& accessor = model.accessors[accessor_index];

	assert(accessor.bufferView < model.bufferViews.size());
	const auto& buffer_view = model.bufferViews[accessor.bufferView];

	assert(buffer_view.buffer < model.buffers.size());
	const auto& buffer = model.buffers[buffer_view.buffer];

	auto stride = accessor.ByteStride(buffer_view);
	auto start_byte = accessor.byteOffset + buffer_view.byteOffset;
	auto end_byte = start_byte + accessor.count * stride;

	return {buffer.data.begin() + start_byte, buffer.data.begin() + end_byte};
}

uint32_t AssetImporter::getAttributeCount(const tinygltf::Model* model, uint32_t accessor_id)
{
	assert(accessor_id < model->accessors.size());

	return static_cast<uint32_t>(model->accessors[accessor_id].count);
}

uint32_t AssetImporter::getAttributeSize(const tinygltf::Model* model, uint32_t accessor_id)
{
	assert(accessor_id < model->accessors.size());
	auto& accessor = model->accessors[accessor_id];

	size_t component_size = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	size_t component_num = tinygltf::GetNumComponentsInType(accessor.type);

	return static_cast<uint32_t>(component_size * component_num);
}

uint32_t AssetImporter::getAttributeStride(const tinygltf::Model* model, uint32_t accessor_id)
{
	assert(accessor_id < model->accessors.size());
	auto& accessor = model->accessors[accessor_id];

	assert(accessor.bufferView < model->bufferViews.size());
	auto& buffer_view = model->bufferViews[accessor.bufferView];

	return static_cast<uint32_t>(accessor.ByteStride(buffer_view));
}

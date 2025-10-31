#pragma once

#include <span>
#include <string_view>

#include <tiny_gltf.h>

#include "Scene/Core/Scene.hpp"
#include "Scene/Core/Node.hpp"
#include "Scene/Components/Mesh.hpp"
#include "Scene/Components/Camera.hpp"
#include "Scene/Components/Light.hpp"
#include "Scene/Behaviours/CameraController.hpp"
#include "Scene/Resources/Material.hpp"

class AssetImporter {
private:
	template <typename S, typename D>
	static std::vector<D> convertData(std::span<const uint8_t> data);

	static std::vector<uint8_t>     getAttributeData(const tinygltf::Model& model, uint32_t accessor_index);
	static std::span<const uint8_t> getAttributeDataView(const tinygltf::Model& model, uint32_t accessor_index);

	static uint32_t getAttributeCount(const tinygltf::Model* model, uint32_t accessor_id);
	static uint32_t getAttributeSize(const tinygltf::Model* model, uint32_t accessor_id);
	static uint32_t getAttributeStride(const tinygltf::Model* model, uint32_t accessor_id);

public:
	static std::unique_ptr<Scene>   loadScene(std::string_view file_path);
	static std::unique_ptr<SubMesh> loadModel(const tinygltf::Model& tfmodel, uint32_t index);

	static std::unique_ptr<Node>     parseNode(const tinygltf::Node& tfnode, size_t index);
	static std::unique_ptr<Mesh>     parseMesh(const tinygltf::Mesh& tfmesh);
	static std::unique_ptr<SubMesh>  parseSubmesh(const tinygltf::Mesh& tfmesh, const tinygltf::Model& model, uint32_t index);
	static std::unique_ptr<Camera>   parseCamera(const tinygltf::Camera& tfcamera);
	static std::unique_ptr<Light>    parseLight(const tinygltf::Light& tflight);
	static std::unique_ptr<Material> parseMaterial(const tinygltf::Material& tfmaterial);
	static std::unique_ptr<Texture>  parseTexture(const tinygltf::Texture& tftexture);

	static std::unique_ptr<Camera>   createDefaultCamera();
	static std::unique_ptr<Light>    createDefaultLight();
	static std::unique_ptr<Material> createDefaultMaterial();

	static std::unique_ptr<CameraController> createDefaultCameraController();
};

template <typename S, typename D>
std::vector<D> AssetImporter::convertData(std::span<const uint8_t> data)
{
	static_assert(sizeof(S) <= sizeof(D),
	              "Source type size must be less than or equal to destination type size");

	const size_t   count = data.size() / sizeof(S);
	std::vector<D> result(count);

	const S* src_ptr = reinterpret_cast<const S*>(data.data());
	for (size_t i = 0; i < count; i++)
		result[i] = static_cast<D>(src_ptr[i]);

	return result;
}

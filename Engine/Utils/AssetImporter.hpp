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

	static std::vector<uint8_t>     getAttributeData(const tinygltf::Model& tfmodel, uint32_t accessor_index);
	static std::span<const uint8_t> getAttributeDataView(const tinygltf::Model& tfmodel, uint32_t accessor_index);

	static uint32_t getAttributeCount(const tinygltf::Model* tfmodel, uint32_t accessor_id);
	static uint32_t getAttributeSize(const tinygltf::Model* tfmodel, uint32_t accessor_id);
	static uint32_t getAttributeStride(const tinygltf::Model* tfmodel, uint32_t accessor_id);

	static std::weak_ptr<Material> default_pbr_material;
	static std::weak_ptr<Texture>  default_base_color_texture;
	static std::weak_ptr<Texture>  default_metallic_roughness_texture;

	static void initDefaultCamera(Scene& scene);
	static void initDefaultLight(Scene& scene);
	static void initDefaultTextures(Scene& scene);
	static void initDefaultMaterials(Scene& scene);
	static void initDefaultCameraController(Scene& scene);

public:
	static std::unique_ptr<Scene> loadScene(std::string_view scene_path);

	static std::unique_ptr<Node>     parseNode(const tinygltf::Node& tfnode);
	static std::unique_ptr<Mesh>     parseMesh(const tinygltf::Mesh& tfmesh);
	static std::unique_ptr<Camera>   parseCamera(const tinygltf::Camera& tfcamera);
	static std::unique_ptr<Light>    parseLight(const tinygltf::Light& tflight);
	static std::shared_ptr<SubMesh>  parseSubmesh(const tinygltf::Mesh& tfmesh, const tinygltf::Model& tfmodel, uint32_t indexm, const std::vector<std::shared_ptr<Material>>& materials);
	static std::shared_ptr<Texture>  parseTexture(const tinygltf::Texture& tftexture, const tinygltf::Model& tfmodel);
	static std::shared_ptr<Material> parseMaterial(const tinygltf::Material& tfmaterial, const tinygltf::Model& tfmodel, const std::vector<std::shared_ptr<Texture>>& textures);

	static std::unique_ptr<Camera>           createDefaultCamera(const std::string& = "Default_Camera");
	static std::unique_ptr<Light>            createDefaultLight(const std::string& = "Default_Light");
	static std::shared_ptr<Texture>          createDefaultTexture(const std::string& = "Default_Texture");
	static std::shared_ptr<Material>         createDefaultMaterial(const std::string& = "Default_Material");
	static std::unique_ptr<CameraController> createDefaultCameraController(const std::string& = "Default_Camera_Controller");
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

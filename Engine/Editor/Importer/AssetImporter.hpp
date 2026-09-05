module;

#include <tiny_gltf.h>

export module Editor:AssetImporter;

import Runtime.World;
import :CameraController;

export namespace Vortex {

class AssetImporter {
private:
	template <typename S, typename D>
	static std::vector<D> convertData(std::span<const uint8> data);

	static std::vector<uint8> getAttributeData(const tinygltf::Model& tfmodel, uint32 accessor_index);
	static std::span<const uint8> getAttributeDataView(const tinygltf::Model& tfmodel, uint32 accessor_index);

	static uint32 getAttributeCount(const tinygltf::Model* tfmodel, uint32 accessor_id);
	static uint32 getAttributeSize(const tinygltf::Model* tfmodel, uint32 accessor_id);
	static uint32 getAttributeStride(const tinygltf::Model* tfmodel, uint32 accessor_id);

	static void initDefaultCameraComponent(Scene& scene);
	static void initDefaultLightComponent(Scene& scene);
	static void initDefaultCameraControllerComponent(Scene& scene);

	static auto getDefaultBaseColorTexture(AssetManager& assets) -> AssetHandle<TextureAsset>;
	static auto getDefaultMetallicRoughnessTexture(AssetManager& assets) -> AssetHandle<TextureAsset>;
	static auto getDefaultMaterial(AssetManager& assets) -> AssetHandle<MaterialAsset>;

public:
	static std::unique_ptr<Scene> loadScene(AssetManager& assets, std::string_view scene_path);

	static std::unique_ptr<Actor> parseActor(const tinygltf::Node& tfnode);
	static std::unique_ptr<MeshComponent> parseMeshComponent(const tinygltf::Mesh& tfmesh);
	static std::unique_ptr<CameraComponent> parseCameraComponent(const tinygltf::Camera& tfcamera);
	static std::unique_ptr<LightComponent> parseLightComponent(const tinygltf::Light& tflight);
	static std::shared_ptr<MeshAsset> parseMeshAsset(const tinygltf::Mesh& tfmesh,
	    const tinygltf::Model& tfmodel,
	    const std::vector<AssetHandle<MaterialAsset>>& materials,
	    AssetHandle<MaterialAsset> default_material,
	    std::string virtual_path);
	static std::shared_ptr<TextureAsset> parseTextureAsset(const tinygltf::Texture& tftexture,
	    const tinygltf::Model& tfmodel,
	    std::string virtual_path);
	static std::shared_ptr<MaterialAsset> parseMaterialAsset(const tinygltf::Material& tfmaterial,
	    const tinygltf::Model& tfmodel,
	    const std::vector<AssetHandle<TextureAsset>>& textures,
	    AssetHandle<TextureAsset> default_base_color,
	    AssetHandle<TextureAsset> default_metallic_roughness,
	    std::string virtual_path);

	static std::unique_ptr<CameraComponent> createDefaultCameraComponent(const std::string& = "DefaultCameraComponent");
	static std::unique_ptr<LightComponent> createDefaultLightComponent(const std::string& = "DefaultLightComponent");
	static std::shared_ptr<TextureAsset> createDefaultTextureAsset(std::string name = "DefaultTexture", std::string virtual_path = {});
	static std::shared_ptr<MaterialAsset> createDefaultMaterialAsset(std::string name = "DefaultMaterial", std::string virtual_path = {});
	static std::unique_ptr<CameraControllerComponent> createDefaultCameraControllerComponent(const std::string& = "DefaultCameraControllerComponent");
};

template <typename S, typename D>
std::vector<D> AssetImporter::convertData(std::span<const uint8> data)
{
	static_assert(sizeof(S) <= sizeof(D),
	    "Source type size must be less than or equal to destination type size");

	const size_t count = data.size() / sizeof(S);
	std::vector<D> result(count);

	const S* src_ptr = reinterpret_cast<const S*>(data.data());
	for (size_t i = 0; i < count; i++)
		result[i] = static_cast<D>(src_ptr[i]);

	return result;
}

}        // namespace Vortex

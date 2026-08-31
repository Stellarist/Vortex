export module Runtime.World:Assets.MaterialAsset;

import Core;
import :Assets.Asset;
import :Assets.TextureAsset;

export namespace Vortex {

enum class MaterialTextureSlot : uint8 {
	BaseColor,
	MetallicRoughness,
	Normal,
	Occlusion,
	Emissive,
	Count,
};

class MaterialAsset : public Asset {
public:
	enum class ShadingModel : uint8 {
		Lit,
		Unlit,
		Count,
	};

	enum class AlphaMode : uint8 {
		Opaque,
		Mask,
		Blend,
		Count,
	};

private:
	ShadingModel shading_model{ShadingModel::Lit};

	Vec4  albedo{1.0f};
	float metallic{0.0f};
	float roughness{0.0f};

	Vec3      emissive{0.0f, 0.0f, 0.0f};
	bool      double_sided{false};
	float     alpha_cutoff{0.5f};
	AlphaMode alpha_mode{AlphaMode::Opaque};

	std::array<AssetHandle<TextureAsset>, static_cast<size_t>(MaterialTextureSlot::Count)> textures;

public:
	MaterialAsset(std::string name, ShadingModel shading_model = ShadingModel::Lit, std::string virtual_path = {});
	~MaterialAsset() override = default;

	auto getShadingModel() const noexcept -> ShadingModel;
	auto setShadingModel(ShadingModel shading_model) noexcept -> MaterialAsset&;

	auto getAlbedo() const noexcept -> Vec4;
	auto setAlbedo(const Vec4& albedo) noexcept -> MaterialAsset&;

	auto getMetallic() const noexcept -> float;
	auto setMetallic(float metallic) noexcept -> MaterialAsset&;

	auto getRoughness() const noexcept -> float;
	auto setRoughness(float roughness) noexcept -> MaterialAsset&;

	auto getEmissive() const noexcept -> Vec3;
	auto setEmissive(const Vec3& emissive) noexcept -> MaterialAsset&;

	bool getDoubleSided() const noexcept;
	auto setDoubleSided(bool double_sided) noexcept -> MaterialAsset&;

	float getAlphaCutoff() const noexcept;
	auto setAlphaCutoff(float alpha_cutoff) noexcept -> MaterialAsset&;

	auto getAlphaMode() const noexcept -> AlphaMode;
	auto setAlphaMode(AlphaMode alpha_mode) noexcept -> MaterialAsset&;

	auto getTextures() const noexcept -> const std::array<AssetHandle<TextureAsset>, static_cast<size_t>(MaterialTextureSlot::Count)>&;

	auto getTexture(MaterialTextureSlot slot) const -> AssetHandle<TextureAsset>;
	auto setTexture(MaterialTextureSlot slot, AssetHandle<TextureAsset> texture) -> MaterialAsset&;
};

}        // namespace Vortex

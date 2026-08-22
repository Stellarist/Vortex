export module Runtime.World:Material;

import Core;
import :Texture;
import :Resource;

export namespace Vortex {

class Material : public Resource {
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

	std::unordered_map<std::string, std::shared_ptr<Texture>> textures;

public:
	Material(ShadingModel shading_model = ShadingModel::Lit, const std::string& name = {});
	~Material() override = default;

	std::type_index getType() override;

	auto getShadingModel() const -> ShadingModel;
	auto setShadingModel(ShadingModel shading_model) -> void;

	auto getAlbedo() const -> Vec4;
	auto setAlbedo(const Vec4& albedo) -> void;

	auto getMetallic() const -> float;
	auto setMetallic(float metallic) -> void;

	auto getRoughness() const -> float;
	auto setRoughness(float roughness) -> void;

	auto getEmissive() const -> Vec3;
	void setEmissive(const Vec3& emissive);

	bool getDoubleSided() const;
	void setDoubleSided(bool double_sided);

	float getAlphaCutoff() const;
	void  setAlphaCutoff(float alpha_cutoff);

	auto getAlphaMode() const -> AlphaMode;
	void setAlphaMode(AlphaMode alpha_mode);

	auto getTextures() -> std::unordered_map<std::string, std::shared_ptr<Texture>>&;
	auto getTextures() const -> const std::unordered_map<std::string, std::shared_ptr<Texture>>&;

	auto getTexture(const std::string& name) const -> std::shared_ptr<Texture>;
	void addTexture(const std::string& name, std::shared_ptr<Texture> texture);
};

}        // namespace Vortex

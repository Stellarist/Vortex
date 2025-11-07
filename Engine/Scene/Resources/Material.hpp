#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

#include "Texture.hpp"
#include "Scene/Core/Resource.hpp"

enum class AlphaMode : uint8_t {
	Opaque,
	Mask,
	Blend,
	Other
};

class Material : public Resource {
protected:
	glm::vec3 emissive{0.0f, 0.0f, 0.0f};
	bool      double_sided{false};
	float     alpha_cutoff{0.5f};
	AlphaMode alpha_mode{AlphaMode::Opaque};

	std::unordered_map<std::string, std::shared_ptr<Texture>> textures;

public:
	Material(const std::string& name);

	~Material() override = default;

	std::type_index getType() override;

	auto getEmissive() -> glm::vec3;
	void setEmissive(const glm::vec3& emissive);

	bool getDoubleSided() const;
	void setDoubleSided(bool double_sided);

	float getAlphaCutoff() const;
	void  setAlphaCutoff(float alpha_cutoff);

	auto getAlphaMode() -> AlphaMode;
	void setAlphaMode(AlphaMode alpha_mode);

	auto getTextures() -> std::unordered_map<std::string, std::shared_ptr<Texture>>&;
	auto getTextures() const -> const std::unordered_map<std::string, std::shared_ptr<Texture>>&;

	auto getTexture(const std::string& name) -> std::shared_ptr<Texture>;
	void addTexture(const std::string& name, std::shared_ptr<Texture> texture);
};

class PBRMaterial : public Material {
private:
	glm::vec4 base_color_factor{1.0f};
	float     metallic_factor{0.0f};
	float     roughness_factor{0.0f};

public:
	PBRMaterial(const std::string& name);
	~PBRMaterial() override = default;

	std::type_index getType() override;

	auto setBaseColorFactor(const glm::vec4& base_color_factor) -> void;
	auto getBaseColorFactor() const -> glm::vec4;

	auto setMetallicFactor(float metallic_factor) -> void;
	auto getMetallicFactor() const -> float;

	auto setRoughnessFactor(float roughness_factor) -> void;
	auto getRoughnessFactor() const -> float;
};

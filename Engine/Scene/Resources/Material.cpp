#include "Material.hpp"

Material::Material(const std::string& name) :
    Resource(name)
{}

std::type_index Material::getType()
{
	return typeid(Material);
}

glm::vec3 Material::getEmissive()
{
	return emissive;
}

void Material::setEmissive(const glm::vec3& emissive)
{
	this->emissive = emissive;
}

bool Material::getDoubleSided() const
{
	return double_sided;
}

void Material::setDoubleSided(bool double_sided)
{
	this->double_sided = double_sided;
}

float Material::getAlphaCutoff() const
{
	return alpha_cutoff;
}

void Material::setAlphaCutoff(float alpha_cutoff)
{
	this->alpha_cutoff = alpha_cutoff;
}

AlphaMode Material::getAlphaMode()
{
	return alpha_mode;
}

void Material::setAlphaMode(AlphaMode alpha_mode)
{
	this->alpha_mode = alpha_mode;
}

auto Material::getTextures() -> std::unordered_map<std::string, std::shared_ptr<Texture>>&
{
	return textures;
}

auto Material::getTextures() const -> const std::unordered_map<std::string, std::shared_ptr<Texture>>&
{
	return textures;
}

std::shared_ptr<Texture> Material::getTexture(const std::string& name)
{
	auto it = textures.find(name);
	return (it != textures.end()) ? it->second : nullptr;
}

void Material::addTexture(const std::string& name, std::shared_ptr<Texture> texture)
{
	textures[name] = std::move(texture);
}

PBRMaterial::PBRMaterial(const std::string& name) :
    Material{name}
{}

std::type_index PBRMaterial::getType()
{
	return typeid(PBRMaterial);
}

void PBRMaterial::setBaseColorFactor(const glm::vec4& base_color_factor)
{
	this->base_color_factor = base_color_factor;
}

glm::vec4 PBRMaterial::getBaseColorFactor() const
{
	return base_color_factor;
}

void PBRMaterial::setMetallicFactor(float metallic_factor)
{
	this->metallic_factor = metallic_factor;
}

float PBRMaterial::getMetallicFactor() const
{
	return metallic_factor;
}

void PBRMaterial::setRoughnessFactor(float roughness_factor)
{
	this->roughness_factor = roughness_factor;
}

float PBRMaterial::getRoughnessFactor() const
{
	return roughness_factor;
}

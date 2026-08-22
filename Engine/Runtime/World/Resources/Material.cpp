module Runtime.World;

namespace Vortex {

Material::Material(Material::ShadingModel shading_model, const std::string& name) :
    Resource(name),
    shading_model(shading_model)
{}

std::type_index Material::getType()
{
	return typeid(Material);
}

Vec4 Material::getAlbedo() const
{
	return albedo;
}

void Material::setAlbedo(const Vec4& albedo)
{
	this->albedo = albedo;
}

float Material::getMetallic() const
{
	return metallic;
}

void Material::setMetallic(float metallic)
{
	this->metallic = metallic;
}

float Material::getRoughness() const
{
	return roughness;
}

void Material::setRoughness(float roughness)
{
	this->roughness = roughness;
}

Vec3 Material::getEmissive() const
{
	return emissive;
}

void Material::setEmissive(const Vec3& emissive)
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

Material::AlphaMode Material::getAlphaMode() const
{
	return alpha_mode;
}

void Material::setAlphaMode(Material::AlphaMode alpha_mode)
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

std::shared_ptr<Texture> Material::getTexture(const std::string& name) const
{
	auto it = textures.find(name);
	return (it != textures.end()) ? it->second : nullptr;
}

void Material::addTexture(const std::string& name, std::shared_ptr<Texture> texture)
{
	textures[name] = std::move(texture);
}

}        // namespace Vortex

module Runtime.Asset;

namespace Vortex {

MaterialAsset::MaterialAsset(std::string asset_name, ShadingModel material_shading_model, std::string asset_path) :
    Asset(std::move(asset_name), std::move(asset_path)),
    shading_model(material_shading_model)
{}

MaterialAsset::ShadingModel MaterialAsset::getShadingModel() const noexcept
{
	return shading_model;
}

MaterialAsset& MaterialAsset::setShadingModel(MaterialAsset::ShadingModel new_shading_model) noexcept
{
	shading_model = new_shading_model;
	touch();
	return *this;
}

Vec4 MaterialAsset::getAlbedo() const noexcept
{
	return albedo;
}

MaterialAsset& MaterialAsset::setAlbedo(const Vec4& new_albedo) noexcept
{
	albedo = new_albedo;
	touch();
	return *this;
}

float MaterialAsset::getMetallic() const noexcept
{
	return metallic;
}

MaterialAsset& MaterialAsset::setMetallic(float new_metallic) noexcept
{
	metallic = new_metallic;
	touch();
	return *this;
}

float MaterialAsset::getRoughness() const noexcept
{
	return roughness;
}

MaterialAsset& MaterialAsset::setRoughness(float new_roughness) noexcept
{
	roughness = new_roughness;
	touch();
	return *this;
}

Vec3 MaterialAsset::getEmissive() const noexcept
{
	return emissive;
}

MaterialAsset& MaterialAsset::setEmissive(const Vec3& new_emissive) noexcept
{
	emissive = new_emissive;
	touch();
	return *this;
}

bool MaterialAsset::getDoubleSided() const noexcept
{
	return double_sided;
}

MaterialAsset& MaterialAsset::setDoubleSided(bool new_double_sided) noexcept
{
	double_sided = new_double_sided;
	touch();
	return *this;
}

float MaterialAsset::getAlphaCutoff() const noexcept
{
	return alpha_cutoff;
}

MaterialAsset& MaterialAsset::setAlphaCutoff(float new_alpha_cutoff) noexcept
{
	alpha_cutoff = new_alpha_cutoff;
	touch();
	return *this;
}

MaterialAsset::AlphaMode MaterialAsset::getAlphaMode() const noexcept
{
	return alpha_mode;
}

MaterialAsset& MaterialAsset::setAlphaMode(MaterialAsset::AlphaMode new_alpha_mode) noexcept
{
	alpha_mode = new_alpha_mode;
	touch();
	return *this;
}

const std::unordered_map<std::string, AssetHandle<TextureAsset>>& MaterialAsset::getTextures() const noexcept
{
	return textures;
}

AssetHandle<TextureAsset> MaterialAsset::getTexture(std::string_view texture_name) const
{
	auto it = textures.find(std::string(texture_name));
	return it == textures.end() ? AssetHandle<TextureAsset>{} : it->second;
}

MaterialAsset& MaterialAsset::setTexture(std::string texture_name, AssetHandle<TextureAsset> texture)
{
	textures.insert_or_assign(std::move(texture_name), std::move(texture));
	touch();
	return *this;
}

}        // namespace Vortex

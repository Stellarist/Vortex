module Runtime.World;

namespace Vortex {

static float clampUnit(float value, float fallback = 0.0f) noexcept
{
	return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : fallback;
}

static float clampPositive(float value) noexcept
{
	return std::isfinite(value) ? std::max(value, 0.0f) : 0.0f;
}

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
	shading_model = new_shading_model < ShadingModel::Count ? new_shading_model : ShadingModel::Lit;
	touch();
	return *this;
}

Vec4 MaterialAsset::getAlbedo() const noexcept
{
	return albedo;
}

MaterialAsset& MaterialAsset::setAlbedo(const Vec4& new_albedo) noexcept
{
	albedo = Vec4{clampUnit(new_albedo.r), clampUnit(new_albedo.g), clampUnit(new_albedo.b), clampUnit(new_albedo.a, 1.0f)};

	touch();
	return *this;
}

float MaterialAsset::getMetallic() const noexcept
{
	return metallic;
}

MaterialAsset& MaterialAsset::setMetallic(float new_metallic) noexcept
{
	metallic = clampUnit(new_metallic);
	touch();
	return *this;
}

float MaterialAsset::getRoughness() const noexcept
{
	return roughness;
}

MaterialAsset& MaterialAsset::setRoughness(float new_roughness) noexcept
{
	roughness = clampUnit(new_roughness, 1.0f);
	touch();
	return *this;
}

Vec3 MaterialAsset::getEmissive() const noexcept
{
	return emissive;
}

MaterialAsset& MaterialAsset::setEmissive(const Vec3& new_emissive) noexcept
{
	emissive = Vec3{clampPositive(new_emissive.r), clampPositive(new_emissive.g), clampPositive(new_emissive.b)};

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
	alpha_cutoff = clampUnit(new_alpha_cutoff, 0.5f);
	touch();
	return *this;
}

MaterialAsset::AlphaMode MaterialAsset::getAlphaMode() const noexcept
{
	return alpha_mode;
}

MaterialAsset& MaterialAsset::setAlphaMode(MaterialAsset::AlphaMode new_alpha_mode) noexcept
{
	alpha_mode = new_alpha_mode < AlphaMode::Count ? new_alpha_mode : AlphaMode::Opaque;
	touch();
	return *this;
}

const std::array<AssetHandle<TextureAsset>, static_cast<size_t>(MaterialTextureSlot::Count)>& MaterialAsset::getTextures() const noexcept
{
	return textures;
}

AssetHandle<TextureAsset> MaterialAsset::getTexture(MaterialTextureSlot slot) const
{
	const auto index = static_cast<size_t>(slot);
	return index < textures.size() ? textures[index] : AssetHandle<TextureAsset>{};
}

MaterialAsset& MaterialAsset::setTexture(MaterialTextureSlot slot, AssetHandle<TextureAsset> texture)
{
	const auto index = static_cast<size_t>(slot);
	CHECK(Range, index < textures.size(), "Material texture slot is out of range");

	textures[index] = std::move(texture);
	touch();
	return *this;
}

}        // namespace Vortex

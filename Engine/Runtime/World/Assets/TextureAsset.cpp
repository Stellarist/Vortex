module Runtime.World;

namespace Vortex {

TextureAsset::TextureAsset(std::string asset_name, Dimension texture_dimension, std::string asset_path) :
    Asset(std::move(asset_name), std::move(asset_path)),
    dimension(texture_dimension)
{}

TextureAsset::Dimension TextureAsset::getDimension() const noexcept
{
	return dimension;
}

TextureAsset& TextureAsset::setDimension(TextureAsset::Dimension new_dimension) noexcept
{
	dimension = new_dimension;
	touch();
	return *this;
}

const std::vector<uint8>& TextureAsset::getData() const noexcept
{
	return data;
}

TextureAsset& TextureAsset::setData(std::vector<uint8> new_data)
{
	data = std::move(new_data);
	touch();
	return *this;
}

uint32 TextureAsset::getFormat() const noexcept
{
	return format;
}

TextureAsset& TextureAsset::setFormat(uint32 new_format) noexcept
{
	format = new_format;
	touch();
	return *this;
}

uint32 TextureAsset::getWidth() const noexcept
{
	return width;
}

TextureAsset& TextureAsset::setWidth(uint32 new_width) noexcept
{
	width = new_width;
	touch();
	return *this;
}

uint32 TextureAsset::getHeight() const noexcept
{
	return height;
}

TextureAsset& TextureAsset::setHeight(uint32 new_height) noexcept
{
	height = new_height;
	touch();
	return *this;
}

bool TextureAsset::valid() const noexcept
{
	return !data.empty() && width > 0 && height > 0 && format > 0;
}

}        // namespace Vortex

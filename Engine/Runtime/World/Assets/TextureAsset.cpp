module Runtime.World;

namespace Vortex {

static uint32 texturePixelFormatByteSize(TexturePixelFormat format) noexcept
{
	switch (format) {
	case TexturePixelFormat::RGBA8:
		return 4;
	default:
		return 0;
	}
}

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
	dimension = new_dimension < Dimension::Count ? new_dimension : Dimension::Tex2D;
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

TexturePixelFormat TextureAsset::getFormat() const noexcept
{
	return format;
}

TextureAsset& TextureAsset::setFormat(TexturePixelFormat new_format) noexcept
{
	format = new_format < TexturePixelFormat::Count ? new_format : TexturePixelFormat::Unknown;
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
	const uint64 byte_size = texturePixelFormatByteSize(format);
	if (width == 0 || height == 0 || byte_size == 0 || dimension >= Dimension::Count)
		return false;

	const uint64 face_count = dimension == Dimension::TexCube ? 6 : 1;
	uint64 expected_size = width;
	for (const uint64 factor : {static_cast<uint64>(height), face_count, byte_size}) {
		if (expected_size > std::numeric_limits<uint64>::max() / factor)
			return false;
		expected_size *= factor;
	}
	return expected_size == data.size();
}

}        // namespace Vortex

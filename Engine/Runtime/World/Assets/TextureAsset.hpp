export module Runtime.World:Assets.TextureAsset;

import Core;
import :Assets.Asset;

export namespace Vortex {

enum class TexturePixelFormat : uint8 {
	Unknown,
	RGBA8,
	Count,
};

class TextureAsset : public Asset {
public:
	enum class Dimension : uint8 {
		Tex2D,
		TexCube,
		Count,
	};

private:
	TexturePixelFormat format{TexturePixelFormat::Unknown};
	Dimension dimension{Dimension::Tex2D};

	std::vector<uint8> data;

	uint32 width{0};
	uint32 height{0};

public:
	TextureAsset(std::string name, Dimension dimension = Dimension::Tex2D, std::string virtual_path = {});
	~TextureAsset() override = default;

	auto getFormat() const noexcept -> TexturePixelFormat;
	auto setFormat(TexturePixelFormat new_format) noexcept -> TextureAsset&;

	auto getDimension() const noexcept -> Dimension;
	auto setDimension(Dimension dimension) noexcept -> TextureAsset&;

	auto getData() const noexcept -> const std::vector<uint8>&;
	auto setData(std::vector<uint8> new_data) -> TextureAsset&;

	auto getWidth() const noexcept -> uint32;
	auto setWidth(uint32 new_width) noexcept -> TextureAsset&;

	auto getHeight() const noexcept -> uint32;
	auto setHeight(uint32 new_height) noexcept -> TextureAsset&;

	bool valid() const noexcept;
};

}        // namespace Vortex

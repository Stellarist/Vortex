export module Runtime.World:Assets.TextureAsset;

import Core;
import :Assets.Asset;

export namespace Vortex {

class TextureAsset : public Asset {
public:
	enum class Dimension : uint8 {
		Tex2D,
		TexCube,
		Count,
	};

private:
	Dimension dimension{Dimension::Tex2D};

	std::vector<uint8> data;

	uint32 format{0};
	uint32 width{0};
	uint32 height{0};

public:
	TextureAsset(std::string name, Dimension dimension = Dimension::Tex2D, std::string virtual_path = {});
	~TextureAsset() override = default;

	auto getDimension() const noexcept -> Dimension;
	auto setDimension(Dimension dimension) noexcept -> TextureAsset&;

	auto getData() const noexcept -> const std::vector<uint8>&;
	auto setData(std::vector<uint8> new_data) -> TextureAsset&;

	auto getFormat() const noexcept -> uint32;
	auto setFormat(uint32 new_format) noexcept -> TextureAsset&;

	auto getWidth() const noexcept -> uint32;
	auto setWidth(uint32 new_width) noexcept -> TextureAsset&;

	auto getHeight() const noexcept -> uint32;
	auto setHeight(uint32 new_height) noexcept -> TextureAsset&;

	bool valid() const noexcept;
};

}        // namespace Vortex

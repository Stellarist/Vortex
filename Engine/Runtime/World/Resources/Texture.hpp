export module Runtime.World:Texture;

import Core;
import :Resource;

export namespace Vortex {

class Texture : public Resource {
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
	Texture(Dimension dimension = Dimension::Tex2D, const std::string& name = {});
	~Texture() override = default;

	std::type_index getType() override;

	auto getDimension() const -> Dimension;
	void setDimension(Dimension dimension);

	auto getData() const -> const std::vector<uint8>&;
	void setData(std::vector<uint8> new_data);

	auto getFormat() const -> uint32;
	void setFormat(uint32 new_format);

	auto getWidth() const -> uint32;
	void setWidth(uint32 new_width);

	auto getHeight() const -> uint32;
	void setHeight(uint32 new_height);

	bool valid() const;
};

}        // namespace Vortex

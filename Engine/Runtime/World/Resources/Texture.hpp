#pragma once

#include <vector>
#include <string>

#include "Runtime/World/Base/Resource.hpp"

class Texture : public Resource {
public:
	enum class Dimension : uint8_t {
		Tex2D,
		TexCube,
		Count,
	};

private:
	Dimension dimension{Dimension::Tex2D};

	std::vector<uint8_t> data;

	uint32_t format{0};
	uint32_t width{0};
	uint32_t height{0};

public:
	Texture(Dimension dimension = Dimension::Tex2D, const std::string& name = {});
	~Texture() override = default;

	std::type_index getType() override;

	auto getDimension() const -> Dimension;
	void setDimension(Dimension dimension);

	auto getData() const -> const std::vector<uint8_t>&;
	void setData(std::vector<uint8_t> new_data);

	auto getFormat() const -> uint32_t;
	void setFormat(uint32_t new_format);

	auto getWidth() const -> uint32_t;
	void setWidth(uint32_t new_width);

	auto getHeight() const -> uint32_t;
	void setHeight(uint32_t new_height);

	bool valid() const;
};

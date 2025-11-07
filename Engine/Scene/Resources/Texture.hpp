#pragma once

#include <vector>
#include <string>

#include "Scene/Core/Resource.hpp"

class Texture : public Resource {
private:
	std::vector<uint8_t> data;

	uint32_t format{0};
	uint32_t width{0};
	uint32_t height{0};

public:
	Texture(const std::string& name);
	~Texture() override = default;

	std::type_index getType() override;

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

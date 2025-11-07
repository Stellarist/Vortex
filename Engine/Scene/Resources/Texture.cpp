#include "Texture.hpp"

Texture::Texture(const std::string& name) :
    Resource(name)
{}

std::type_index Texture::getType()
{
	return typeid(Texture);
}

const std::vector<uint8_t>& Texture::getData() const
{
	return data;
}

void Texture::setData(std::vector<uint8_t> new_data)
{
	data = std::move(new_data);
}

uint32_t Texture::getFormat() const
{
	return format;
}

void Texture::setFormat(uint32_t new_format)
{
	format = new_format;
}

uint32_t Texture::getWidth() const
{
	return width;
}

void Texture::setWidth(uint32_t new_width)
{
	width = new_width;
}

uint32_t Texture::getHeight() const
{
	return height;
}

void Texture::setHeight(uint32_t new_height)
{
	height = new_height;
}

bool Texture::valid() const
{
	return !data.empty() && width > 0 && height > 0 && format > 0;
}

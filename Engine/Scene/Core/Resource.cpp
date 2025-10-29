#include "Resource.hpp"

Resource::Resource(std::string name) :
    name(std::move(name))
{}

const std::string& Resource::getName() const
{
	return name;
}

const std::string& Resource::getPath() const
{
	return path;
}

void Resource::setPath(const std::string& new_path)
{
	path = new_path;
}

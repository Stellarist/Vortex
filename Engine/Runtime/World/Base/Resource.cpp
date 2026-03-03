#include "Resource.hpp"

Resource::Resource(std::string name) :
    name(std::move(name))
{}

const std::string& Resource::getName() const
{
	return name;
}

module Runtime.World;

namespace Vortex {

std::atomic<uint64> Object::id_counter = 0;

Object::Object(std::string object_name) noexcept :
    name(std::move(object_name)), uid(id_counter++)
{}

uint64 Object::getUid() const noexcept
{
	return uid;
}

const std::string& Object::getName() const noexcept
{
	return name;
}

std::type_index Object::getType() const noexcept
{
	return typeid(*this);
}

}        // namespace Vortex

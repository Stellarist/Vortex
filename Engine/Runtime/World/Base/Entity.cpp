module Runtime.World;

namespace Vortex {

std::atomic<uint64> Entity::id_counter = 0;

Entity::Entity() :
    uid(id_counter++)
{}

uint64 Entity::getUid() const
{
	return uid;
}

}        // namespace Vortex
